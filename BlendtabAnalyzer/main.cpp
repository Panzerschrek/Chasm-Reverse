#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include "../Common/files.hpp"
#include "../Common/palette.hpp"
#include "../Common/tga.hpp"
using namespace ChasmReverse;

static void ConvertBlendTable( const char* const in_table_file, const char* const out_tga_file )
{
	Palette palette;
	LoadPalette( palette );

	unsigned char blend_tab[ 256u * 255u ];

	std::FILE* const file= std::fopen( in_table_file, "rb" );
	if( file == nullptr )
	{
		std::cout << "Can not open " << in_table_file << std::endl;
		return;
	}

	FileRead( file, blend_tab, sizeof(blend_tab) );
	std::fclose( file );

	const unsigned int c_color_width= 64u;
	unsigned char extended_blend_tab[ ( 256u + c_color_width ) * ( 255u + c_color_width ) ]= {0};

	for( unsigned int y= 0u; y < 255u; y++ )
	for( unsigned int x= 0u; x < 256u; x++ )
		extended_blend_tab[ c_color_width + x + ( y + c_color_width ) * ( 256u + c_color_width ) ]= blend_tab[ x + y * 256u ];

	for( unsigned int y= 0u; y < 255u; y++ )
	for( unsigned int x= 0u; x < c_color_width; x++ )
		extended_blend_tab[ x + ( y + c_color_width ) * ( 256u + c_color_width ) ]= y;

	for( unsigned int y= 0u; y < c_color_width; y++ )
	for( unsigned int x= 0u; x < 256u; x++ )
		extended_blend_tab[ x + c_color_width + y * ( 256u + c_color_width ) ]= x;

	WriteTGA( 256u + c_color_width, 255u + c_color_width, extended_blend_tab, palette.data(), out_tga_file );
}

int main()
{
	ConvertBlendTable( "CSM.BIN.depacked/CHASM.RGB", "CHASM.tga" );
	ConvertBlendTable( "CSM.BIN.depacked/CHASM60.RGB", "CHASM60.tga" );
}
