#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include "../Common/files.hpp"
#include "../Common/palette.hpp"
#include "../Common/tga.hpp"
using namespace ChasmReverse;

int main( const int argc, const char* const argv[] )
{
	const char* file_name= "WORM.CAR";
	if( argc > 1 )
		file_name= argv[1];

	FILE* const file= std::fopen( file_name, "rb" );
	if( file == nullptr )
	{
		std::cout << "Could not read file \"" << file_name << "\"" << std::endl;
		return -1;
	}

	std::fseek( file, 0, SEEK_END );
	const unsigned int file_size= std::ftell( file );
	std::fseek( file, 0, SEEK_SET );

	std::vector<unsigned char> file_content( file_size );
	FileRead( file, file_content.data(), file_content.size() );

	std::fclose( file );

	Palette palette;
	LoadPalette( palette );

	const unsigned int c_texture_bytes_offset= 0x486Au;
	const unsigned int c_texture_offset= 0x486Cu;

	unsigned short texture_size;
	std::memcpy( &texture_size, file_content.data() + c_texture_bytes_offset, sizeof(unsigned short) );

	unsigned int texture_height= texture_size / 64u;

	WriteTGA(
		64u,
		texture_height,
		( file_content.data() + c_texture_offset ),
		palette.data(),
		( std::string( file_name ) + ".texture.tga" ).c_str() );
}
