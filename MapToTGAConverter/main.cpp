#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include "../Common/files.hpp"
#include "../Common/tga.hpp"
using namespace ChasmReverse;

static void GenPalette(unsigned char* palette )
{
	for( unsigned int i= 0; i < 768; i++ )
		palette[i]= i / 3 * 2;
		//palette[i]= std::rand();
}

int main( const int argc, const char* const argv[] )
{
	const unsigned int c_width= 256;
	const unsigned int c_height= 256;

	const char* map_file_name= "MAP.01";
	if( argc > 1 )
		map_file_name= argv[1];

	char out_file_name[256];
	std::strcpy( out_file_name, map_file_name );
	std::strcpy( out_file_name + std::strlen( map_file_name ), ".tga" );

	FILE* const file= std::fopen( map_file_name, "rb" );
	if( file == nullptr )
	{
		std::cout << "Could not read file \"" << map_file_name << "\"" << std::endl;
		return -1;
	}

	std::vector<unsigned char> file_data( c_width * c_height );

	FileRead( file, file_data.data(), file_data.size() );

	std::fclose( file );

	for( unsigned int y= 0; y < c_height; y++ )
	for( unsigned int x= 0; x < c_width / 2; x++ )
		std::swap(
			file_data[ y * c_width + x ],
			file_data[ y * c_width + c_width - 1u - x ] );


	unsigned char palette[768];
	GenPalette( palette );

	WriteTGA( c_width, c_height, file_data.data(), palette, out_file_name );
}
