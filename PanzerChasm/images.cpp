#include <cmath>

#include "assert.hpp"
#include "vfs.hpp"

#include "images.hpp"

namespace PanzerChasm
{

void ConvertToRGBA(
	const unsigned int pixel_count,
	const unsigned char* const in_data,
	const Palette& palette,
	unsigned char* const out_data,
	const unsigned char transpareny_color_index )
{
	for( unsigned int p= 0u; p < pixel_count; p++ )
	{
		for( unsigned int j= 0u; j < 3u; j++ )
			out_data[ p * 4u + j ]= palette[ in_data[p] * 3u + j ];

		out_data[ p * 4u + 3 ]= in_data[p] == transpareny_color_index ? 0u : 255u;
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

	// Make color correction.
	// TODO - do color correction as screen image postprocessing.
	const float c_mix_k= 1.3f;
	const float c_one_minux_mix_k= 1.0f - c_mix_k;

	for( unsigned int i= 0u; i < 256u; i++ )
	{
		float rgb[3];
		for( unsigned int j= 0u; j < 3u; j++ )
			rgb[j]= float( out_palette[ i * 3u + j ] );
		const float grey= ( rgb[0] + rgb[1] + rgb[2] ) / 3.0f;

		for( unsigned int j= 0u; j < 3u; j++ )
		{
			const int c= static_cast<int>( std::round( rgb[j] * c_mix_k + grey * c_one_minux_mix_k ) );
			out_palette[ i * 3u + j ]= std::max( std::min( c, 255 ), 0 );
		}
	}
}

void CreateConsoleBackground(
	const Size2& size,
	const Vfs& vfs,
	std::vector<unsigned char>& out_data_indexed )
{
	const Vfs::FileContent texture_file= vfs.ReadFile( "CONSOLE.CEL" );
	const CelTextureHeader& header= *reinterpret_cast<const CelTextureHeader*>( texture_file.data() );
	const unsigned char* const texture_data= texture_file.data() + sizeof(CelTextureHeader);

	out_data_indexed.resize( size.Width() * size.Height(), 0u );

	const auto fill_rect=
	[&](
		unsigned int src_x, unsigned int src_y,
		unsigned int dst_x, unsigned int dst_y,
		unsigned int width,unsigned int height )
	{
		PC_ASSERT( src_x + width  <= header.size[0] );
		PC_ASSERT( src_y + height <= header.size[1] );
		PC_ASSERT( dst_x + width  <= size.Width () );
		PC_ASSERT( dst_y + height <= size.Height() );

		for( unsigned int y= 0u; y < height; y++ )
		for( unsigned int x= 0u; x < width ; x++ )
		{
			const unsigned char color_index= texture_data[ src_x + x + ( src_y + y ) * header.size[0] ];
			unsigned char* const dst= out_data_indexed.data() + ( dst_x + x + ( dst_y + y ) * size.Width() );
			*dst= color_index;
		}
	};

	const unsigned int c_left_border= 4u;
	const unsigned int c_right_border= 5u;
	const unsigned int c_upper_border= 4u;
	const unsigned int c_bottom_border= 16u;

	const unsigned int c_src_texture_x_start= 94u;

	const unsigned int main_field_width = header.size[0] - ( 94u + c_left_border ) - c_right_border;
	const unsigned int main_field_height= header.size[1] - c_upper_border - c_bottom_border;

	// Lower left corner
	fill_rect(
		c_src_texture_x_start, header.size[1] - c_bottom_border,
		0u, size.Height() - c_bottom_border,
		c_left_border, c_bottom_border );

	// Lower right corner
	fill_rect(
		header.size[0] - c_right_border, header.size[1] - c_bottom_border,
		size.Width() - c_right_border, size.Height() - c_bottom_border,
		c_right_border, c_bottom_border );

	// Upper left corner
	fill_rect(
		c_src_texture_x_start, 0u,
		0u, 0u,
		c_left_border, c_upper_border );

	// Upper right corner
	fill_rect(
		header.size[0] - c_right_border, 0u,
		size.Width() - c_right_border, 0u,
		c_right_border, c_upper_border );

	// Up and down borders
	for( unsigned int x= c_left_border; x < size.Width() - c_right_border; x+= main_field_width )
	{
		const unsigned int width= std::min( main_field_width, size.Width() - c_right_border - x );
		fill_rect(
			c_src_texture_x_start + c_left_border, header.size[1] - c_bottom_border,
			x, size.Height() - c_bottom_border,
			width, c_bottom_border );

		fill_rect(
			c_src_texture_x_start + c_left_border, 0u,
			x, 0u,
			width, c_upper_border );
	}

	// Left and right borders
	for( unsigned int y= c_upper_border; y < size.Height() - c_bottom_border; y+= main_field_height )
	{
		const unsigned int height= std::min( main_field_height, size.Height () - c_bottom_border - y );
		fill_rect(
			c_src_texture_x_start, c_upper_border,
			0u, y,
			c_left_border, height );

		fill_rect(
			header.size[0] - c_right_border, c_upper_border,
			size.Width() - c_right_border, y,
			c_right_border, height );
	}

	// Main field
	for( unsigned int y= c_upper_border; y < size.Height() - c_bottom_border; y+= main_field_height )
	for( unsigned int x= c_left_border ; x < size.Width () - c_right_border ; x+= main_field_width  )
	{
		const unsigned int width = std::min( main_field_width , size.Width () - c_right_border  - x );
		const unsigned int height= std::min( main_field_height, size.Height() - c_bottom_border - y );

		fill_rect(
			c_src_texture_x_start + c_left_border, c_upper_border,
			x, y,
			width, height );
	}
}

void CreateConsoleBackgroundRGBA(
	const Size2& size,
	const Vfs& vfs,
	const Palette& palette,
	std::vector<unsigned char>& out_data_rgba )
{
	std::vector<unsigned char> data_indexed;
	CreateConsoleBackground( size, vfs, data_indexed );
	out_data_rgba.resize( data_indexed.size() * 4u );

	for( unsigned int i= 0u; i < data_indexed.size(); i++ )
	{
		const unsigned char color_index= data_indexed[i];
		out_data_rgba[ i * 4u + 0u ]= palette[ color_index * 3u + 0u ];
		out_data_rgba[ i * 4u + 1u ]= palette[ color_index * 3u + 1u ];
		out_data_rgba[ i * 4u + 2u ]= palette[ color_index * 3u + 2u ];
		out_data_rgba[ i * 4u + 3u ]= 255u;
	}
}

} // namespace PanzerChasm
