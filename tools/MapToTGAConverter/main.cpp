#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include "common/files.hpp"
#include "common/tga.hpp"
using namespace ChasmReverse;

static void GenPalette(unsigned char* palette )
{
	for( unsigned int i= 0; i < 768; i++ )
		palette[i]= i / 3;
		//palette[i]= std::rand();
}

int main( const int argc, const char* const argv[] )
{
	const unsigned int c_map_size= 64u;
	const unsigned int c_lightmap_scale= 4u;
	const unsigned int c_lightmap_size[2u]= { c_lightmap_scale * c_map_size, c_lightmap_scale * c_map_size };

	const char* map_file_name= "MAP.01";
	if( argc > 1 )
		map_file_name= argv[1];


	FILE* const file= std::fopen( map_file_name, "rb" );
	if( file == nullptr )
	{
		std::cout << "Could not read file \"" << map_file_name << "\"" << std::endl;
		return -1;
	}

	std::vector<unsigned char> file_data( 0x27001u );
	FileRead( file, file_data.data(), file_data.size() );

	std::fclose( file );

	std::vector<unsigned char> lightmap_bock( c_lightmap_size[0u] * c_lightmap_size[1u] );
	std::memcpy(
		lightmap_bock.data(),
		file_data.data() + 0x01u,
		lightmap_bock.size() );

	for( unsigned int y= 0; y < c_lightmap_size[1u]; y++ )
	for( unsigned int x= 0; x < c_lightmap_size[0u] / 2u; x++ )
		std::swap(
			lightmap_bock[ y * c_lightmap_size[0u] + x ],
			lightmap_bock[ y * c_lightmap_size[0u] + c_lightmap_size[0u] - 1u - x ] );

	std::vector<unsigned char> walls_block( c_map_size * c_map_size * 11u );
	for( unsigned int b= 0; b < 11; b++ )
	{
		const unsigned char* const src= file_data.data() + 0x18001u;
		unsigned char* const dst= walls_block.data() + c_map_size * c_map_size * b;
		for( unsigned int y= 0u; y < c_map_size; y++ )
		for( unsigned int x= 0u; x < c_map_size; x++ )
			dst[ x + y * c_map_size ]= src[ ( x + y * c_map_size ) * 11u + b ];
	}

	std::vector<unsigned char> second_lightmap_block( c_map_size * c_map_size * 8u );
	for( unsigned int b= 0u; b < 8u; b++ )
	{
		for( unsigned int y= 0u; y < c_map_size; y++ )
		{
			const unsigned char* const src= file_data.data() + 0x10001u + 8u * c_map_size * y;
			unsigned char* const dst= second_lightmap_block.data() + c_map_size * c_map_size * b + c_map_size * y;

			for( unsigned int x= 0u; x < c_map_size; x++ )
				dst[x]= src[ (c_map_size - 1u - x ) + b ];
		}
	}

	unsigned char palette[768];
	GenPalette( palette );

	WriteTGA(
		c_lightmap_size[0u], c_lightmap_size[1u],
		lightmap_bock.data(),
		palette,
		( std::string(map_file_name) + ".lightmap.tga" ).c_str() );

	WriteTGA(
		c_map_size, c_map_size * 11u,
		walls_block.data(),
		palette,
		( std::string(map_file_name) + ".walls.tga" ).c_str() );

	WriteTGA(
		c_map_size, c_map_size * 8u,
		second_lightmap_block.data(),
		palette,
		( std::string(map_file_name) + ".second_lightmap.tga" ).c_str() );

	WriteTGA(
		c_map_size, c_map_size * 4u,
		file_data.data() + 0x23001u,
		palette,
		( std::string(map_file_name) + ".maps.tga" ).c_str() );

}
