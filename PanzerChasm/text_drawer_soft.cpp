#include <cstring>

#include "assert.hpp"
#include "game_resources.hpp"
#include "images.hpp"
#include "vfs.hpp"

#include "text_drawer_soft.hpp"

namespace PanzerChasm
{

TextDrawerSoft::TextDrawerSoft(
	const RenderingContextSoft& rendering_context,
	const GameResources& game_resources )
	: rendering_context_(rendering_context)
{
	const Vfs::FileContent font_file= game_resources.vfs->ReadFile( FontParams::font_file_name );
	const CelTextureHeader* const cel_header= reinterpret_cast<const CelTextureHeader*>( font_file.data() );
	const unsigned char* const font_data= font_file.data() + sizeof(CelTextureHeader);

	const unsigned int pixel_count= cel_header->size[0] * cel_header->size[1];

	for( unsigned int c= 0u; c < FontParams::colors_variations; c++ )
	{
		unsigned char* const font_data_shifted= font_texture_data_ + FontParams::atlas_width * FontParams::atlas_height * c;

		ColorShift(
			FontParams::start_inner_color, FontParams::end_inner_color,
			FontParams::color_shifts[c],
			pixel_count,
			font_data,
			font_data_shifted );

		// Hack. Move "slider" letter from code '\0';
		for( unsigned int y= 0; y < FontParams::letter_place_height; y++ )
			std::memcpy(
				font_data_shifted + FontParams::atlas_width * y + FontParams::letter_place_width * ( c_slider_back_letter_code & 15u ) +  FontParams::atlas_width * FontParams::letter_place_height * ( c_slider_back_letter_code >> 4u ),
				font_data_shifted + FontParams::atlas_width * y,
				FontParams::letter_place_width );
	}

	CalculateLettersWidth(
		font_data,
		letters_width_ );

	letters_width_[ c_slider_back_letter_code ]= letters_width_[0]; // Move slider letter.
}

TextDrawerSoft::~TextDrawerSoft()
{}

unsigned int TextDrawerSoft::GetLineHeight() const
{
	return FontParams::letter_height;
}

void TextDrawerSoft::Print(
	const int x, const int y,
	const char* const text,
	const unsigned int scale,
	const FontColor color,
	const Alignment alignment )
{
	const PaletteTransformed& palette= *rendering_context_.palette_transformed;
	uint32_t* const dst_pixels= rendering_context_.window_surface_data;
	const int dst_row_pixels= rendering_context_.row_pixels;
	const int max_x= int( rendering_context_.viewport_size.Width () );
	const int max_y= int( rendering_context_.viewport_size.Height() );

	const int scale_i= int(scale);
	const int d_tc_v= int(color) * int(FontParams::atlas_height);

	int current_y= y;

	const char* c= text;
	while( 1 )
	{
		if( *c == '\0' )
			break;

		unsigned int line_width= 0u;
		const char* line_end_c= c;
		while( ! ( *line_end_c == '\n' || *line_end_c == '\0' ) )
		{
			line_width+= letters_width_[ int(*line_end_c) ];
			line_end_c++;
		}

		int current_x;
		if( alignment == Alignment::Center )
			current_x= x - int( scale_i * line_width / 2u );
		else if( alignment == Alignment::Right )
			current_x= x - int( scale_i * line_width );
		else
			current_x= x;

		while( !( *c == '\n' || *c == '\0' ) )
		{
			const int letter_width= int( letters_width_[int(*c)] );

			const int glyph_tc_u= FontParams::letter_place_width  * ( int(*c) & 15 ) + FontParams::letter_u_offset;
			const int glyph_tc_v= FontParams::letter_place_height * ( int(*c) >> 4 ) + FontParams::letter_v_offset + d_tc_v;

			for( int glyph_y= 0; glyph_y < FontParams::letter_place_height - FontParams::letter_v_offset; glyph_y++ )
			for( int glyph_x= 0; glyph_x < letter_width; glyph_x++ )
			{
				const unsigned char color_index=
					font_texture_data_[
						( glyph_tc_u + glyph_x ) +
						( glyph_tc_v + glyph_y ) * int(FontParams::atlas_width) ];
				if( color_index == 255u )
					continue;

				const uint32_t color= palette[color_index];

				for( int sy= 0; sy < scale_i; sy++ )
				{
					const int dst_y= current_y + glyph_y * scale_i + sy;
					if( dst_y < 0 || dst_y >= max_y ) continue;

					for( int sx= 0; sx < scale_i; sx++ )
					{
						const int dst_x= current_x + glyph_x * scale_i + sx;
						if( dst_x < 0 || dst_y >= max_x ) continue;

						dst_pixels[ dst_x + dst_y * dst_row_pixels ]= color;
					}
				}
			}

			current_x+= letter_width * scale_i;
			c++;
		}
		if( *c == '\n' ) c++;

		current_y+= scale * int(FontParams::letter_height);
	}
}

} // namespace PanzerChasm
