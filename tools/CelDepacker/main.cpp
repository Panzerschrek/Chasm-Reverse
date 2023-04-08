#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

#include "common/files.hpp"
#include "common/tga.hpp"
using namespace ChasmReverse;

#pragma pack(push, 1)

struct Header
{
	char id[2];
	unsigned short width;
	unsigned short height;
	unsigned char reserved[32-6];
};

static_assert( sizeof(Header) == 32, "Invalid size" );

#pragma pack(pop)

static void ShiftPalette( unsigned char* const palette )
{
	for( unsigned int i= 0; i < 768; i++ )
	{
		const int shifted= palette[i] << 2;
		palette[i]= std::min( shifted, 255 );
	}
}

int main( const int argc, const char* const argv[] )
{
	const char* in_file_name= "";
	const char* out_file_name= "";

	for( int i= 1; i < argc; i++ )
	{
		const char* const arg= argv[i];
		if( std::strcmp( arg, "-i" ) == 0 )
		{
			if( i < argc - 1 )
				in_file_name= argv[ i + 1 ];
			else
				std::cout << "Error, expected file name, after -i" << std::endl;
		}
		else if( std::strcmp( arg, "-o" ) == 0 )
		{
			if( i < argc - 1 )
				out_file_name= argv[ i + 1 ];
			else
				std::cout << "Error, expected file name, after -o" << std::endl;
		}
	}

	if( std::strlen( in_file_name ) == 0 )
	{
		std::cout << "Error, no input file" << std::endl;
		return -1;
	}

	char out_file_name_buffer[ 1024 ];
	if( std::strlen( out_file_name ) == 0 )
	{
		const int in_file_name_len= std::strlen( in_file_name );
		int i= in_file_name_len;

		while( i >= 0 && in_file_name[i] != '.' )
			i--;

		if( i >= 0 )
		{
			std::memcpy( out_file_name_buffer, in_file_name, i + 1 );
			std::memcpy( out_file_name_buffer + i + 1, "tga", 4 );
		}
		else
		{
			std::memcpy( out_file_name_buffer, in_file_name, in_file_name_len );
			std::memcpy( out_file_name_buffer + in_file_name_len, ".tga", 5 );
		}

		out_file_name= out_file_name_buffer;
	}

	std::FILE* file= std::fopen( in_file_name, "rb" );
	if( file == nullptr )
	{
		std::cout << "Could not read file \"" << in_file_name << "\"" << std::endl;
		return -1;
	}

	Header header;
	FileRead( file, &header, sizeof(header) );

	unsigned char palette[768];
	FileRead( file, palette, sizeof(palette) );
	ShiftPalette( palette );

	std::vector<unsigned char> file_data( header.width * header.height );
	FileRead( file, file_data.data(), file_data.size() );

	std::fclose( file );

	WriteTGA( header.width, header.height, file_data.data(), palette, out_file_name );
}
