#include <cstring>

#include "../assert.hpp"

#include "hud_drawer_soft.hpp"

namespace PanzerChasm
{

HudDrawerSoft::HudDrawerSoft(
	const GameResourcesConstPtr& game_resources,
	const RenderingContextSoft& rendering_context,
	const SharedDrawersPtr& shared_drawers )
	: HudDrawerBase( game_resources, shared_drawers )
	, rendering_context_(rendering_context)
{
	CreateCrosshairTexture( crosshair_image_.size, crosshair_image_.data );

	LoadImage( c_weapon_icons_image_file_name, weapon_icons_image_ );
	LoadImage( c_hud_numbers_image_file_name, hud_numbers_image_ );
	LoadImage( c_hud_background_image_file_name, hud_background_image_ );

	// Mark transparent pixels for numbers.
	for( unsigned char& color_index : hud_numbers_image_.data )
		if( color_index == 0u ) color_index= 255u;
}

HudDrawerSoft::~HudDrawerSoft()
{}

void HudDrawerSoft::DrawCrosshair()
{
	const PaletteTransformed& palette= *rendering_context_.palette_transformed;
	uint32_t* const dst_pixels= rendering_context_.window_surface_data;
	const int dst_row_pixels= rendering_context_.row_pixels;

	const unsigned int x_start= ( rendering_context_.viewport_size.Width () - crosshair_image_.size[0] * scale_ ) >> 1u;
	const unsigned int y_start= ( ( rendering_context_.viewport_size.Height() - crosshair_image_.size[1] * scale_ ) >> 1u ) + scale_ * c_cross_offset;

	for( unsigned int y= 0u; y < crosshair_image_.size[1]; y++ )
	for( unsigned int x= 0u; x < crosshair_image_.size[0]; x++ )
	{
		const unsigned char color_index= crosshair_image_.data[ x + y * crosshair_image_.size[1] ];
		if( color_index == 0u ) continue;
		const uint32_t color= palette[ color_index ];

		for( unsigned int dy= 0u; dy < scale_; dy++ )
		for( unsigned int dx= 0u; dx < scale_; dx++ )
		{
			uint32_t* const dst_pixel=
				&dst_pixels[
					  x_start + x * scale_ + dx +
					( y_start + y * scale_ + dy ) * dst_row_pixels ];

			// Make fast average color calculation.
			*dst_pixel= ( ( ( *dst_pixel ^ color ) & 0xFEFEFEFEu ) >> 1u ) + ( *dst_pixel & color );
		}
	}
}

void HudDrawerSoft::DrawHud( const bool draw_second_hud, const char* const map_name )
{
	const PaletteTransformed& palette= *rendering_context_.palette_transformed;
	uint32_t* const dst_pixels= rendering_context_.window_surface_data;
	const int dst_row_pixels= rendering_context_.row_pixels;
	const Size2& viewport_size= rendering_context_.viewport_size;

	const auto draw_image=
	[&]( const unsigned int x_start, const unsigned int y_start,
		const unsigned char* const img_data, const unsigned int img_data_width,
		const unsigned int width, const unsigned int height )
	{
		for( unsigned int y= 0u; y < height; y++ )
		for( unsigned int x= 0u; x < width ; x++ )
		{
			const unsigned char color_index= img_data[ x + y * img_data_width ];
			if( color_index == 255u )
				continue;

			const uint32_t color= palette[ color_index ];

			for( unsigned int dy= 0u; dy < scale_; dy++ )
			{
				const unsigned int dst_y= y_start + y * scale_ + dy;
				PC_ASSERT( dst_y < viewport_size.Height() );
				for( unsigned int dx= 0u; dx < scale_; dx++ )
				{
					const unsigned int dst_x= x_start + x * scale_ + dx;
					PC_ASSERT( dst_x < viewport_size.Width () );
					dst_pixels[ dst_x + dst_y * dst_row_pixels ]= color;
				}
			}
		}
	};

	const unsigned int hud_x= viewport_size.Width() / 2u - hud_background_image_.size[0] * scale_ / 2u;
	const unsigned int hud_bg_y_offset= draw_second_hud ? ( c_net_hud_line_height + c_hud_line_height ) : c_net_hud_line_height;

	draw_image(
		hud_x, viewport_size.Height() - c_hud_line_height * scale_,
		hud_background_image_.data.data() + hud_bg_y_offset * hud_background_image_.size[0], hud_background_image_.size[0],
		hud_background_image_.size[0], c_hud_line_height );

	if( !draw_second_hud ) // Weapon icon
	{
		const unsigned int icon_width= weapon_icons_image_.size[0] / 8u;
		const unsigned int c_additional_y_border= 1u;

		// TODO - check this
		draw_image(
			hud_x + ( c_weapon_icon_x_offset + c_weapon_icon_border ) * scale_, // x
			viewport_size.Height() - ( weapon_icons_image_.size[1] + c_weapon_icon_y_offset - c_additional_y_border ) * scale_, // y
			weapon_icons_image_.data.data() + icon_width * current_weapon_number_ + c_weapon_icon_border + c_additional_y_border * weapon_icons_image_.size[0], // data
			weapon_icons_image_.size[0], // img data width
			icon_width - c_weapon_icon_border * 2u, // width
			weapon_icons_image_.size[1] - c_weapon_icon_border * 2u /* height */ );
	}

	const auto draw_number=
	[&]( const unsigned int x_end, const unsigned int number, const unsigned int red_value )
	{
		char digits[8];
		std::snprintf( digits, sizeof(digits), "%d", number );
		const unsigned int digit_count= std::strlen(digits);

		const unsigned int digit_width = hud_numbers_image_.size[0] / 10u;
		const unsigned int digit_height= ( hud_numbers_image_.size[1] - 1u ) / 2u;
		const unsigned int y= viewport_size.Height() - ( ( c_hud_line_height - digit_height ) / 2u + digit_height ) * scale_;
		const unsigned int tc_y= number >= red_value ? 0u : digit_height + 1u;

		for( unsigned int d= 0u; d < digit_count; d++ )
		{
			draw_image(
				x_end - scale_ * digit_width * ( digit_count - d ), // x
				y,
				hud_numbers_image_.data.data() + ( digits[d] - '0' ) * digit_width + tc_y * hud_numbers_image_.size[0], // data
				hud_numbers_image_.size[0], // data width
				digit_width - 1u,
				digit_height );
		}
	};

	draw_number( hud_x + scale_ * c_health_x_offset, player_state_.health, c_health_red_value );
	if( !draw_second_hud )
	{
		if( current_weapon_number_ != 0u )
			draw_number( hud_x + scale_ * c_ammo_x_offset, player_state_.ammo[ current_weapon_number_ ], c_ammo_red_value );
		draw_number( hud_x + scale_ * c_armor_x_offset, player_state_.armor, c_armor_red_value );
	}

	if( draw_second_hud )
		HudDrawerBase::DrawKeysAndStat( hud_x, map_name );
}

void HudDrawerSoft::LoadImage( const char* const file_name, Image& out_image )
{
	const Vfs::FileContent texture_file= game_resources_->vfs->ReadFile( file_name );
	const CelTextureHeader& cel_header= *reinterpret_cast<const CelTextureHeader*>( texture_file.data() );

	out_image.size[0]= cel_header.size[0];
	out_image.size[1]= cel_header.size[1];

	const unsigned int pixel_count= out_image.size[0] * out_image.size[1];
	out_image.data.resize( pixel_count );
	std::memcpy( out_image.data.data(), texture_file.data() + sizeof(CelTextureHeader), pixel_count );
}

} // namespace PanzerChasm
