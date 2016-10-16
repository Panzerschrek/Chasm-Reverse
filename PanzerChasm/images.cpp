#include "images.hpp"

namespace PanzerChasm
{

void ConvertToRGBA(
	const unsigned int pixel_count,
	const unsigned char* const in_data,
	const Palette& palette,
	unsigned char* const out_data )
{
	for( unsigned int p= 0u; p < pixel_count; p++ )
	{
		for( unsigned int j= 0u; j < 3u; j++ )
			out_data[ p * 4u + j ]= palette[ in_data[p] * 3u + j ];

		out_data[ p * 4u + 3 ]= in_data[p] == 255u ? 0u : 255u;
	}
}

void FlipAndConvertToRGBA(
	const unsigned int width, const unsigned int height,
	const unsigned char* const in_data,
	const Palette& palette,
	unsigned char* const out_data )
{
	for( unsigned int y= 0u; y < height; y++ )
	{
		const unsigned char* const src= in_data + (height - y - 1u) * width;
		unsigned char* const dst= out_data + 4u * y * width;
		for( unsigned int x= 0u; x < width; x++ )
		{
			for( unsigned int j= 0u; j < 3u; j++ )
				dst[ x * 4u + j ]= palette[ src[x] * 3u + j ];

			dst[ x * 4u + 3 ]= src[x] == 255u ? 0u : 255u;
		}
	}
}

void ColorShift(
	unsigned char start_color, unsigned char end_color,
	char shift,
	unsigned int pixel_count,
	const unsigned char* in_data,
	unsigned char* out_data )
{
	for( unsigned int i= 0u; i < pixel_count; i++ )
	{
		const unsigned char c= in_data[i];
		if( c >= start_color && c < end_color )
			out_data[i]= (unsigned char)( int(c) + int(shift) );
		else
			out_data[i]= c;
	}
}

void LoadPalette(
	const Vfs& vfs,
	Palette& out_palette )
{
	const Vfs::FileContent palette_file= vfs.ReadFile( "CHASM2.PAL" );

	// Convert from 6-bit to 8-bit.
	for( unsigned int i= 0u; i < 768u; i++ )
		out_palette[i]= palette_file[i] << 2u;
}

} // namespace PanzerChasm
