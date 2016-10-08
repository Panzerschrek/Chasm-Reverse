#include <iostream>

#include "files.hpp"

#include "tga.hpp"

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
	tga.colormap_type= 1; // image with colormap
	tga.image_type= 1; // image with palette without compression

	tga.colormap_index= 0;
	tga.colormap_length= 256;
	tga.colormap_size= 32;

	tga.x_origin= 0;
	tga.y_origin= 0;
	tga.width = width ;
	tga.height= height;

	tga.pixel_bits= 8;
	tga.attributes= 1 << 5; // vertical flip flag
	tga.attributes|= 8; // bits in alpha-channell

	std::FILE* file= std::fopen( file_name, "wb" );
	if( file == nullptr )
	{
		std::cout << "Could not create file \"" << file_name << "\"" << std::endl;
		return;
	}

	FileWrite( file, &tga, sizeof(tga) );

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

	std::fclose( file );
}

} // namespace ChasmReverse
