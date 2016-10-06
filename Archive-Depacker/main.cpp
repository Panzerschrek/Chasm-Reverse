#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

// TODO - detect byte order
#define LittleInt2(x) (x)
#define LittleInt4(x) (x)

const constexpr unsigned int g_file_name_length= 12u;

#pragma pack(push, 1)
struct FileInfoPacked
{
	unsigned char name_length;
	char name[ g_file_name_length ]; // without end null
	unsigned int size;
	unsigned int offset;
};
#pragma pack(pop)

struct FileInfo
{
	char name[ g_file_name_length + 1u ]; // null terminated
	unsigned int size;
	unsigned int offset;
};

static void DepackFileInfo( const FileInfoPacked& file_info_packed, FileInfo& out_file_info )
{
	std::memcpy( out_file_info.name, file_info_packed.name, file_info_packed.name_length );
	out_file_info.name[ file_info_packed.name_length ]= '\0';

	std::memcpy( &out_file_info.size  , &file_info_packed.size  , sizeof(unsigned int) );
	std::memcpy( &out_file_info.offset, &file_info_packed.offset, sizeof(unsigned int) );

	out_file_info.size  = LittleInt4( out_file_info.size   );
	out_file_info.offset= LittleInt4( out_file_info.offset );
}

static void FileRead( std::FILE* const file, void* buffer, const unsigned int size )
{
	unsigned int read_total= 0u;

	do
	{
		const int read= std::fread( buffer, 1, size, file );
		if( read <= 0 )
			break;

		read_total+= read;
	} while( read_total < size );
}

static void FileWrite( std::FILE* const file, const void* buffer, const unsigned int size )
{
	unsigned int write_total= 0u;

	do
	{
		const int write= std::fwrite( buffer, 1, size, file );
		if( write <= 0 )
			break;

		write_total+= write;
	} while( write_total < size );
}

int main(const int argc, const char* const argv[])
{
	const char* archive_file_name= "CSM.BIN";
	const char* out_dir= "";

	for( int i= 1; i < argc; i++ )
	{
		const char* const arg= argv[i];
		if( std::strcmp( arg, "-i" ) == 0 )
		{
			if( i < argc - 1 )
				archive_file_name= argv[ i + 1 ];
			else
				std::cout << "Error, expected file name, after -i" << std::endl;
		}
		else if( std::strcmp( arg, "-o" ) == 0 )
		{
			if( i < argc - 1 )
				out_dir= argv[ i + 1 ];
			else
				std::cout << "Error, expected directory, after -i" << std::endl;
		}
	}

	std::FILE* const file= std::fopen( archive_file_name, "rb" );
	if( file == nullptr )
	{
		std::cout << "Could not read file \"" << archive_file_name << "\"" << std::endl;
		return -1;
	}

	char header[4];
	FileRead( file, header, sizeof(header) );
	if( std::strncmp( header, "CSid", sizeof(header) ) != 0 )
	{
		std::cout << "File \"" << archive_file_name << "\" is not \"Chasm: The Rift\" archive" << std::endl;
		return -1;
	}

	unsigned short files_in_archive_count;
	FileRead( file, &files_in_archive_count, sizeof(files_in_archive_count) );
	files_in_archive_count= LittleInt2(files_in_archive_count);

	std::cout << "Files count: " << files_in_archive_count << std::endl << std::endl;

	std::vector<FileInfoPacked>	files_info_packed( files_in_archive_count );

	FileRead( file, files_info_packed.data(), files_info_packed.size() * sizeof(FileInfoPacked ) );

	std::vector<unsigned char> file_buffer;

	for( const FileInfoPacked& file_info_packed : files_info_packed )
	{
		FileInfo file_info;
		DepackFileInfo( file_info_packed, file_info );

		file_buffer.resize( file_info.size );

		std::fseek( file, file_info.offset, SEEK_SET );
		FileRead( file, file_buffer.data(), file_buffer.size() );

		std::cout << "Write \"" << file_info.name << "\"" << std::endl;

		std::string out_file_name( out_dir );
		if( !out_file_name.empty() && out_file_name.back() != '/' )
			out_file_name+= "/";
		out_file_name+= file_info.name;

		std::FILE* file_to_save= std::fopen( out_file_name.c_str(), "wb" );
		if( file_to_save == nullptr )
		{
			std::cout << "Could not write file \"" << out_file_name << "\"" << std::endl;
			continue;
		}

		FileWrite( file_to_save, file_buffer.data(), file_buffer.size() );
		std::fclose( file_to_save );
	}

	std::fclose( file );
}
