#include <iostream>

#include "files.hpp"

#include "tga.hpp"
#include <vector>
#include <array>
#include <cstdint>
#include <cstring>

namespace ChasmReverse
{

#pragma pack(push, 1)
struct TGAHeader
{
	unsigned char id_length;
	unsigned char colormap_type;
	unsigned char image_type;

	unsigned short colormap_index;
	unsigned short colormap_length;
	unsigned char colormap_size;

	unsigned short x_origin;
	unsigned short y_origin;
	unsigned short width ;
	unsigned short height;

	unsigned char pixel_bits;
	unsigned char attributes;
};
#pragma pack(pop)

static_assert( sizeof(TGAHeader) == 18, "Invalid size" );

void WriteTGA(
	const unsigned short width, const unsigned short height,
	const unsigned char* const data,
	const unsigned char* const palette,
	const char* const file_name )
{
	TGAHeader tga;

	tga.id_length= 0;
	tga.colormap_type= (palette == nullptr) ? 0 : 1; // image with/out colormap
	tga.image_type= (palette == nullptr) ? 2 : 1; // image with/out palette without compression

	tga.colormap_index= 0;
	tga.colormap_length= (palette == nullptr) ? 0 : 256;
	tga.colormap_size= 32;

	tga.x_origin= 0;
	tga.y_origin= 0;
	tga.width = width ;
	tga.height= height;

	tga.pixel_bits= (palette == nullptr) ? 32 : 8;
	tga.attributes= (palette == nullptr) ? 0 : 1 << 5; // vertical flip flag
	tga.attributes|= 8; // bits in alpha-channell

	std::FILE* file= std::fopen( file_name, "wb" );
	if( file == nullptr )
	{
		std::cout << "Could not create file \"" << file_name << "\"" << std::endl;
		return;
	}

	FileWrite( file, &tga, sizeof(tga) );

	if(palette != nullptr)
	{
		unsigned char palette_rb_swapped[ 256 * 4 ];
		for( unsigned int i= 0; i < 256; i++ )
		{
			palette_rb_swapped[ i * 4 + 0 ]= palette[ i * 3 + 2 ];
			palette_rb_swapped[ i * 4 + 1 ]= palette[ i * 3 + 1 ];
			palette_rb_swapped[ i * 4 + 2 ]= palette[ i * 3 + 0 ];
			palette_rb_swapped[ i * 4 + 3 ]= 255;
		}
		palette_rb_swapped[255 * 4 + 3]= 0;


		FileWrite( file, palette_rb_swapped, sizeof(palette_rb_swapped) );
		FileWrite( file, data, width * height );
	}
	else
	{
		std::vector<std::array<uint8_t, 4>> pixels(width * height);
		size_t i = 0;
		for(auto & p : pixels)
			p = { data[ i * 4 + 2 ], data[ i * 4 + 1 ], data[ i * 4 + 0 ], data[ i++ * 4 + 3 ] };
		FileWrite( file, &pixels.front().front(), pixels.size() * pixels.front().size() * sizeof(pixels.front().front()) );
	}
	std::fclose( file );
}

} // namespace ChasmReverse
