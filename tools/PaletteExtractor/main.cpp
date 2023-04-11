#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include "common/files.hpp"
#include "common/palette.hpp"
#include "common/tga.hpp"
using namespace ChasmReverse;

int main( const int argc, const char* const argv[] )
{
	const unsigned int c_scale= 8u;
	const unsigned int c_width= 16;
	const unsigned int c_height= 16u;

	Palette palette;
	LoadPalette( palette );

	unsigned char data[ c_scale * c_scale * c_width * c_height ];
	for( unsigned int y= 0u; y < c_height; y++ )
	for( unsigned int sy= 0u; sy < c_scale; sy++ )
	{
		unsigned char* const dst= data + ( y * c_scale + sy ) * c_scale * c_width;
		for( unsigned int x= 0u; x < c_width; x++ )
		for( unsigned int sx= 0u; sx < c_scale; sx++ )
			dst[ x * c_scale + sx ]= x + y * c_width;
	}

	WriteTGA(
		c_width  * c_scale,
		c_height * c_scale,
		data,
		palette.data(),
		"palette.tga" );

}
