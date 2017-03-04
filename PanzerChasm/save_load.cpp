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

} // namespace PanzerChasm
