#include <cctype>
#include <cstring>

#include "common/files.hpp"
using namespace ChasmReverse;

#include "log.hpp"

#include "save_load.hpp"


namespace PanzerChasm
{

const char SaveHeader::c_expected_id[8]= "PanChSv"; // PanzerChasmSave

static unsigned int g_crc_table[256u];

static void InitCRCTable()
{
	unsigned int crc;

	for(unsigned int i= 0u; i < 256u; i++ )
	{
		crc= i;
		for(unsigned int j= 0u; j < 8u; j++ )
			crc= ( ( crc & 1u ) != 0u ) ? ( (crc>>1u) ^ 0xEDB88320u ) : ( crc >> 1u );

		g_crc_table[i]= crc;
	}
}

// TODO - init table at compile time.
static const bool g_crc_table_init=
[]
{
	InitCRCTable();
	return true;
} ();

SaveHeader::HashType SaveHeader::CalculateHash( const unsigned char* data, unsigned int data_size )
{
	// Calculate CRC-32 here.

	unsigned int crc= 0xFFFFFFFFu;
	for( unsigned int i= 0u; i < data_size; i++ )
	{
		crc= g_crc_table[ ( crc ^ data[i] ) & 0xFFu ] ^ (crc >> 8u);
	}

	return crc;
}

bool SaveData(
	const std::filesystem::path& file_name,
	const SaveComment& save_comment,
	const SaveLoadBuffer& data )
{
	FILE* f= std::fopen( file_name.native().c_str(), "wb" );
	if( f == nullptr )
	{
		Log::Warning( "Can not write save \"", file_name.native(), "\"" );
		return false;
	}

	SaveHeader header;
	std::memcpy( header.id, SaveHeader::c_expected_id, sizeof(header.id) );
	header.version= SaveHeader::c_expected_version;
	header.content_size= data.size();
	header.content_hash= SaveHeader::CalculateHash( data.data(), data.size() );

	FileWrite( f, &header, sizeof(SaveHeader) );
	FileWrite( f, save_comment.data(), sizeof(SaveComment) );
	FileWrite( f, data.data(), data.size() );

	std::fclose(f);
	return true;
}

// Returns true, if all ok
bool LoadData(
	const std::filesystem::path& file_name,
	SaveLoadBuffer& out_data )
{
	FILE* const f= std::fopen( file_name.native().c_str(), "rb" );
	if( f == nullptr )
	{
		Log::Warning( "Can not read save \"", file_name.native(), "\"." );
		return false;
	}

	std::fseek( f, 0, SEEK_END );
	const unsigned int file_size= std::ftell( f );
	std::fseek( f, 0, SEEK_SET );

	if( file_size < sizeof(SaveHeader) + sizeof(SaveComment) )
	{
		Log::Warning( "Save file is broken - it is too small." );
		std::fclose(f);
		return false;
	}

	SaveHeader header;
	FileRead( f, &header, sizeof(SaveHeader) );

	if( std::memcmp( header.id, header.c_expected_id, sizeof(header.id) ) != 0 )
	{
		Log::Warning( "Save file is not a PanzerChasm save." );
		std::fclose(f);
		return false;
	}
	if( header.version != SaveHeader::c_expected_version )
	{
		Log::Warning( "Save file has different version." );
		std::fclose(f);
		return false;
	}

	const unsigned int content_size= file_size - ( sizeof(SaveHeader) + sizeof(SaveComment) );
	if( header.content_size != content_size )
	{
		Log::Warning( "Save file has content size, different from actual file size." );
		std::fclose(f);
		return false;
	}

	SaveComment save_comment;
	FileRead( f, save_comment.data(), sizeof(SaveComment) );

	out_data.resize( content_size );
	FileRead( f, out_data.data(), out_data.size() );

	if( header.content_hash != SaveHeader::CalculateHash( out_data.data(), out_data.size() ) )
	{
		out_data.clear();

		Log::Warning( "Save file is broken - saved content hash is different from actual content hash." );
		std::fclose(f);
		return false;
	}

	std::fclose(f);
	return true;
}

bool LoadSaveComment(
	const std::filesystem::path& file_name,
	SaveComment& out_save_comment )
{
	FILE* const f= std::fopen( file_name.native().c_str(), "rb" );
	if( f == nullptr )
	{
		return false;
	}

	std::fseek( f, 0, SEEK_END );
	const unsigned int file_size= std::ftell( f );
	std::fseek( f, 0, SEEK_SET );

	if( file_size < sizeof(SaveHeader) + sizeof(SaveComment) )
	{
		std::fclose(f);
		return false;
	}

	std::fseek( f, sizeof(SaveHeader), SEEK_SET );
	FileRead( f, out_save_comment.data(), sizeof(SaveComment) );

	std::fclose(f);
	return true;
}

std::filesystem::path GetSaveFileNameForSlot(const uint8_t slot_number)
{
	static char tmp[12] = "save_";
	std::snprintf( tmp, sizeof(tmp), "save_%02hhu.pcs", slot_number );
	return std::filesystem::absolute(SAVES_DIR / tmp);
}

std::filesystem::path GetScreenShotFileNameForSlot( const uint8_t slot_number, const std::filesystem::path& shot_name )
{
	static char tmp[12] = "shot_";
	std::snprintf( tmp, sizeof(tmp), "%4s_%02hhu.tga", shot_name.native().c_str(), slot_number );
	return std::filesystem::absolute(SAVES_DIR / tmp);
}

uint8_t GetScreenShotSlotNumber( const std::filesystem::path& saves_dir )
{
	static uint8_t slot_number = 0;
	int8_t slot_disk = -1;
	for( const auto & entry : std::filesystem::directory_iterator( std::filesystem::absolute( SAVES_DIR ) ) )
	{
		if( entry.path().extension() == ".tga" )
		{
			const std::filesystem::path& shot_name = remove_extension(entry.path().filename());
			if( shot_name.native().size() > 6 )
			{
				const char digits[3] = { shot_name.native().c_str()[5], shot_name.native().c_str()[6], '\0' };
				if( isdigit( digits[0] ) && isdigit( digits[1] ) )
				{
					const uint8_t num = atoi( digits );
					if( num > slot_disk ) slot_disk = num;

				}
			}
		}

	}

	slot_disk++;
	if(slot_disk == 0) return 0;

	if(slot_disk > 99)
		slot_number = slot_number > 99 ? 0 : slot_number;
	else
		slot_number = slot_disk;

	return slot_number++;
}

void CreateSlotSavesDir()
{
	if(!std::filesystem::exists(SAVES_DIR) && !std::filesystem::create_directories( SAVES_DIR ))
		Log::Warning("Couldn't create saves directory: ", strerror(errno));
}

} // namespace PanzerChasm
