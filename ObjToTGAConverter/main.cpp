#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include "../Common/files.hpp"
#include "../Common/palette.hpp"
#include "../Common/tga.hpp"
using namespace ChasmReverse;

struct SpriteFrame
{
	unsigned short size[2u];
	unsigned short unknown;
	std::vector<unsigned char> data;
};

int main( const int argc, const char* const argv[] )
{
	const char* file_name= "FLAME.OBJ";
	if( argc > 1 )
		file_name= argv[1];

	FILE* const file= std::fopen( file_name, "rb" );
	if( file == nullptr )
	{
		std::cout << "Could not read file \"" << file_name << "\"" << std::endl;
		return -1;
	}

	Palette palette;
	LoadPalette( palette );

	unsigned short frame_count;
	FileRead( file, &frame_count, sizeof(unsigned short) );

	unsigned int max_size[2u]= { 0u, 0u };
	std::vector< SpriteFrame > frames( frame_count );

	for( SpriteFrame& frame : frames )
	{
		FileRead( file, &frame.size[0u], sizeof(unsigned short) );
		FileRead( file, &frame.size[1u], sizeof(unsigned short) );
		FileRead( file, &frame.unknown, sizeof(unsigned short) );

		frame.data.resize( frame.size[0u] * frame.size[1u] );

		FileRead( file, frame.data.data(), frame.data.size() );

		for( unsigned int j= 0u; j < 2u; j++ )
			if( frame.size[j] > max_size[j] ) max_size[j]= frame.size[j];
	}

	for( unsigned int j= 0u; j < 2u; j++ )
	{
		max_size[j]+= 2u; // add offsets
		max_size[j]= ( max_size[j] + 15u ) & (~15u);
	}

	std::vector<unsigned char> atlas( max_size[0u] * max_size[1u] * frame_count, 255u );

	for( unsigned int frame= 0u; frame < frame_count; frame++ )
	{
		const unsigned char* const src= frames[ frame ].data.data();
		unsigned char* const dst= atlas.data() + max_size[0u] * max_size[1u] * frame + 1u + max_size[0u];

		for( unsigned int y= 0u; y < frames[ frame ].size[1u]; y++ )
		for( unsigned int x= 0u; x < frames[ frame ].size[0u]; x++ )
			dst[ x + y * max_size[0u] ]= src[ y + x * frames[ frame ].size[1u] ];
	}

	WriteTGA(
		max_size[0u], max_size[1u] * frame_count,
		atlas.data(),
		palette.data(),
		( std::string(file_name) + ".tga" ).c_str() );

	std::fclose( file );
}
