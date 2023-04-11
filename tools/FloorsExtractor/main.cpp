#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include "common/files.hpp"
#include "common/palette.hpp"
#include "common/tga.hpp"
using namespace ChasmReverse;

const constexpr unsigned int g_header_size= 64;

const constexpr unsigned int g_floor_texture_size= 64;
const constexpr unsigned int g_floor_texture_data_size= g_floor_texture_size * 86;
const constexpr unsigned int g_floors_in_file= 64;

static std::vector<unsigned char> RepackFloors( const std::vector<unsigned char>& in_floors )
{
	std::vector<unsigned char> result;

	if( in_floors.size() != g_floors_in_file * g_floor_texture_data_size )
	{
		std::cout << "Invalid floors file size" << std::endl;
		return result;
	}

	const unsigned int c_floor_bytes= g_floor_texture_size * g_floor_texture_size;
	const unsigned int c_out_floor_bytes= g_floor_texture_size * g_floor_texture_size * 3 / 2;

	result.resize( g_floors_in_file * c_out_floor_bytes, 0 );

	for( unsigned int floor= 0; floor < g_floors_in_file; floor++ )
	{
		const unsigned char* const in= in_floors.data() + floor * g_floor_texture_data_size;
		const unsigned char* const mip_in1= in + c_floor_bytes;
		const unsigned char* const mip_in2= mip_in1 + c_floor_bytes /  4;
		const unsigned char* const mip_in3= mip_in2 + c_floor_bytes / 16;

		unsigned char* const out= result.data() + floor * c_out_floor_bytes;
		unsigned char* const mip_out1= out + c_floor_bytes;
		unsigned char* const mip_out2= mip_out1 + g_floor_texture_size / 2;
		unsigned char* const mip_out3= mip_out2 + g_floor_texture_size / 4;

		std::memcpy( out, in, c_floor_bytes );

		for( unsigned int y= 0; y < (g_floor_texture_size >> 1); y++ )
		for( unsigned int x= 0; x < (g_floor_texture_size >> 1); x++ )
			mip_out1[ y * g_floor_texture_size + x ]= mip_in1[ y * (g_floor_texture_size >> 1) + x ];

		for( unsigned int y= 0; y < (g_floor_texture_size >> 2); y++ )
		for( unsigned int x= 0; x < (g_floor_texture_size >> 2); x++ )
			mip_out2[ y * g_floor_texture_size + x ]= mip_in2[ y * (g_floor_texture_size >> 2) + x ];

		for( unsigned int y= 0; y < (g_floor_texture_size >> 3); y++ )
		for( unsigned int x= 0; x < (g_floor_texture_size >> 3); x++ )
			mip_out3[ y * g_floor_texture_size + x ]= mip_in3[ y * (g_floor_texture_size >> 3) + x ];
	}

	return result;
}

int main( const int argc, const char* const argv[] )
{
	const char* floors_file_name= "FLOORS.01";
	if( argc > 1 )
		floors_file_name= argv[1];

	char out_file_name[256];
	std::strcpy( out_file_name, floors_file_name );
	std::strcpy( out_file_name + std::strlen( floors_file_name ), ".tga" );

	FILE* const file= std::fopen( floors_file_name, "rb" );
	if( file == nullptr )
	{
		std::cout << "Could not read file \"" << floors_file_name << "\"" << std::endl;
		return -1;
	}

	std::vector<unsigned char> file_data( g_floors_in_file * g_floor_texture_data_size );

	std::fseek( file, g_header_size, SEEK_SET );
	FileRead( file, file_data.data(), file_data.size() );

	std::fclose( file );

	Palette palette;
	LoadPalette( palette );

	const std::vector<unsigned char> floors_repacked= RepackFloors( file_data );

	WriteTGA(
		g_floor_texture_size, floors_repacked.size() / g_floor_texture_size,
		floors_repacked.data(),
		palette.data(),
		out_file_name );
}
