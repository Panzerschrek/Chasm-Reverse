#include <cstring>

#include "assert.hpp"
#include "game_constants.hpp"
#include "game_resources.hpp"
#include "images.hpp"
#include "menu_drawers_common.hpp"
#include "vfs.hpp"

#include "menu_drawer_soft.hpp"

namespace PanzerChasm
{

MenuDrawerSoft::MenuDrawerSoft(
	const RenderingContextSoft& rendering_context,
	const GameResources& game_resources )
	: rendering_context_( rendering_context )
	, menu_scale_( CalculateMenuScale( rendering_context.viewport_size ) )
	, console_scale_( CalculateConsoleScale( rendering_context.viewport_size ) )
{
	Vfs::FileContent file_content;

	const auto load_picture=
	[&]( const char* const file_name, Picture& out_picture )
	{
		game_resources.vfs->ReadFile( file_name, file_content );
		const CelTextureHeader* const cel_header=
			reinterpret_cast<const CelTextureHeader*>( file_content.data() );

		out_picture.size[0]= cel_header->size[0];
		out_picture.size[1]= cel_header->size[1];

		const unsigned int pixel_count= out_picture.size[0] * out_picture.size[1];
		out_picture.data.resize( pixel_count );

		std::memcpy(
			out_picture.data.data(),
			file_content.data() + sizeof(CelTextureHeader),
			pixel_count );
	};

	load_picture( MenuParams::tile_picture, tiles_picture_ );
	load_picture( MenuParams::loading_picture, loading_picture_ );
	load_picture( MenuParams::background_picture, game_background_picture_ );
	load_picture( MenuParams::pause_picture, pause_picture_ );
	load_picture( MenuParams::player_torso_picture, player_torso_picutre_ );

	for( unsigned int i= 0u; i < size_t(MenuPicture::PicturesCount); i++ )
	{
		Picture& pic= menu_pictures_[i];

		game_resources.vfs->ReadFile( MenuParams::menu_pictures[i], file_content );

		const CelTextureHeader* const cel_header=
			reinterpret_cast<const CelTextureHeader*>( file_content.data() );

		const unsigned int height_with_border= cel_header->size[1] + MenuParams::menu_picture_horizontal_border * 2u;

		const unsigned int in_pixel_count= cel_header->size[0] * cel_header->size[1];
		const unsigned int out_pixel_count= cel_header->size[0] * height_with_border;
		pic.data.resize( MenuParams::menu_pictures_shifts_count * out_pixel_count );

		pic.size[0]= cel_header->size[0];
		pic.size[1]= height_with_border * MenuParams::menu_pictures_shifts_count;

		for( unsigned int s= 0u; s < MenuParams::menu_pictures_shifts_count; s++ )
		{
			unsigned char* const pic_data_shifted= pic.data.data() + s * out_pixel_count;
			const unsigned int border_size_bytes= MenuParams::menu_picture_horizontal_border * cel_header->size[0];

			ColorShift(
				MenuParams::start_inner_color, MenuParams::end_inner_color,
				MenuParams::menu_pictures_shifts[ s ],
				in_pixel_count,
				file_content.data() + sizeof(CelTextureHeader),
				pic_data_shifted + border_size_bytes );

			// Fill up and down borders
			std::memset(
				pic_data_shifted,
				0,
				border_size_bytes );
			std::memset(
				pic_data_shifted + out_pixel_count - border_size_bytes,
				0,
				border_size_bytes );
		}
	} // for menu pictures

	const Size2 console_size(
		rendering_context_.viewport_size.Width () / console_scale_,
		rendering_context_.viewport_size.Height() / console_scale_ );

	CreateConsoleBackground(
		console_size, *game_resources.vfs, console_background_picture_.data );

	console_background_picture_.size[0]= console_size.Width ();
	console_background_picture_.size[1]= console_size.Height();

	const Size2 viewport_size_scaled(
		rendering_context_.viewport_size.Width () / menu_scale_,
		rendering_context_.viewport_size.Height() / menu_scale_ );

	Size2 briefbar_picture_size;

	CreateBriefbarTexture(
		viewport_size_scaled, *game_resources.vfs, briefbar_picture_.data, briefbar_picture_size );

	briefbar_picture_.size[0]= briefbar_picture_size.Width ();
	briefbar_picture_.size[1]= briefbar_picture_size.Height();
}

MenuDrawerSoft::~MenuDrawerSoft()
{}

Size2 MenuDrawerSoft::GetViewportSize() const
{
	return rendering_context_.viewport_size;
}

unsigned int MenuDrawerSoft::GetMenuScale() const
{
	return menu_scale_;
}

unsigned int MenuDrawerSoft::GetConsoleScale() const
{
	return console_scale_;
}

Size2 MenuDrawerSoft::GetPictureSize( const MenuPicture picture ) const
{
	const Picture& pic= menu_pictures_[ size_t(picture) ];
	return Size2( pic.size[0], pic.size[1] / MenuParams::menu_pictures_shifts_count );
}

void MenuDrawerSoft::DrawMenuBackground(
	const int start_x, const int start_y,
	const unsigned int width, const unsigned int height )
{
	const PaletteTransformed& palette= *rendering_context_.palette_transformed;
	uint32_t* const dst_pixels= rendering_context_.window_surface_data;
	const int dst_row_pixels= rendering_context_.row_pixels;

	// TODO - rewrite this method.
	// Use more fast tiles texturing. Accept tiles texture with any size, not only 64x64.
	// Draw cool framing, like in MenuDrawerGL.

	const Picture& pic= tiles_picture_;
	PC_ASSERT( pic.size[0] == 64u && pic.size[1] == 64u );

	const int start_x_corrected= start_x - int( MenuParams::menu_border ) * int(menu_scale_);
	const int end_x= start_x_corrected + int(width ) + int( MenuParams::menu_border * 2u ) * int(menu_scale_);
	const int start_y_corrected= start_y - int( MenuParams::menu_border + MenuParams::menu_caption ) * int(menu_scale_);
	const int end_y= start_y_corrected + int(height) + int( MenuParams::menu_border * 2u + MenuParams::menu_caption ) * int(menu_scale_);

	// Tone borders
	const unsigned char c_low= 128u;
	const unsigned char c_middle= 192u;
	const unsigned char c_height= 255u;

	for( int y= start_y_corrected; y < end_y; y++ )
	{
		PC_ASSERT( y >= 0 && y < int(rendering_context_.viewport_size.Height()) );

		const int tc_y= y / int(menu_scale_);
		const int tc_y_warped= tc_y & 63;

		for( int x= start_x_corrected; x < end_x; x++ )
		{
			PC_ASSERT( x >= 0 && x < int(rendering_context_.viewport_size.Width()) );

			const int tc_x= x / int(menu_scale_);
			const int tc_x_warped= tc_x & 63;

			unsigned char components[4];
			std::memcpy( components, &palette[ pic.data[ tc_y_warped * 64 + tc_x_warped ] ], sizeof(uint32_t) );
			for( unsigned int j= 0u; j < 3u; j++ )
				components[j]= ( components[j] * c_middle ) >> 8u;
			std::memcpy( &dst_pixels[ x + y * dst_row_pixels ], components, sizeof(uint32_t) );
		}
	}

	const auto toned_texel_fetch=
	[&]( const int x, const int y, const unsigned char tone ) -> uint32_t
	{
		PC_ASSERT( x >= 0 && y < int(rendering_context_.viewport_size.Width()) );
		PC_ASSERT( y >= 0 && y < int(rendering_context_.viewport_size.Height()) );
		const int tc_y= y / int(menu_scale_);
		const int tc_y_warped= tc_y & 63;
		const int tc_x= x / int(menu_scale_);
		const int tc_x_warped= tc_x & 63;

		unsigned char components[4];
		std::memcpy( components, &palette[ pic.data[ tc_y_warped * 64 + tc_x_warped ] ], sizeof(uint32_t) );
		for( unsigned int j= 0u; j < 3u; j++ )
			components[j]= ( components[j] * tone ) >> 8u;
		uint32_t result;
		std::memcpy( &result, components, sizeof(uint32_t) );
		return result;
	};

	// Upper border.
	for( int y= start_y_corrected; y < start_y_corrected + int(menu_scale_); y++ )
	for( int x= start_x_corrected; x < end_x; x++ )
		dst_pixels[ x + y * dst_row_pixels ]= toned_texel_fetch( x, y, c_height );
	// Upper border inner.
	for( int y= start_y_corrected + int(menu_scale_*12u); y < start_y_corrected + int(menu_scale_*12u) + int(menu_scale_); y++ )
	for( int x= start_x_corrected + int(menu_scale_* 2u); x < end_x - int(menu_scale_* 2u); x++ )
		dst_pixels[ x + y * dst_row_pixels ]= toned_texel_fetch( x, y, c_low );
	// Lower border.
	for( int y= end_y - int(menu_scale_); y < end_y; y++ )
	for( int x= start_x_corrected; x < end_x; x++ )
		dst_pixels[ x + y * dst_row_pixels ]= toned_texel_fetch( x, y, c_low );
	// Lower border inner.
	for( int y= end_y - int(menu_scale_ * 3u); y < end_y - int(menu_scale_ * 2u); y++ )
	for( int x= start_x_corrected + int(menu_scale_* 2u); x < end_x - int(menu_scale_* 2u); x++ )
		dst_pixels[ x + y * dst_row_pixels ]= toned_texel_fetch( x, y, c_height );
	// Left border.
	for( int y= start_y_corrected + int(menu_scale_); y < end_y - int(menu_scale_); y++ )
	for( int x= start_x_corrected; x < start_x_corrected + int(menu_scale_); x++ )
		dst_pixels[ x + y * dst_row_pixels ]= toned_texel_fetch( x, y, c_height );
	// Left border inner.
	for( int y= start_y_corrected + int(menu_scale_*13u); y < end_y - int(menu_scale_*2u); y++ )
	for( int x= start_x_corrected + int(menu_scale_* 2u); x < start_x_corrected + int(menu_scale_*3u); x++ )
		dst_pixels[ x + y * dst_row_pixels ]= toned_texel_fetch( x, y, c_low );
	// Right border.
	for( int y= start_y_corrected + int(menu_scale_); y < end_y - int(menu_scale_); y++ )
	for( int x= end_x - int(menu_scale_); x < end_x; x++ )
		dst_pixels[ x + y * dst_row_pixels ]= toned_texel_fetch( x, y, c_low );
	// Right border inner.
	for( int y= start_y_corrected + int(menu_scale_*13u); y < end_y - int(menu_scale_*2u); y++ )
	for( int x= end_x - int(menu_scale_*3u); x < end_x  - int(menu_scale_*2u); x++ )
		dst_pixels[ x + y * dst_row_pixels ]= toned_texel_fetch( x, y, c_height );
}

void MenuDrawerSoft::DrawMenuPicture(
	const int start_x, const int start_y,
	const MenuPicture picture,
	const PictureColor* const rows_colors )
{
	const PaletteTransformed& palette= *rendering_context_.palette_transformed;
	uint32_t* const dst_pixels= rendering_context_.window_surface_data;
	const int dst_row_pixels= rendering_context_.row_pixels;

	const Picture& pic= menu_pictures_[ static_cast<size_t>(picture) ] ;

	const int picture_height= pic.size[1] / MenuParams::menu_pictures_shifts_count;
	const int row_count= int( pic.size[1] / ( MenuParams::menu_picture_row_height * MenuParams::menu_pictures_shifts_count ) );

	for( int row= 0; row < row_count; row++ )
	{
		const unsigned char* const src= pic.data.data() + int(pic.size[0]) * picture_height * int(rows_colors[row]);

		for( int y= row * int(MenuParams::menu_picture_row_height); y < (row+1) * int(MenuParams::menu_picture_row_height); y++ )
		{
			for( int x= 0; x < int(pic.size[0]); x++ )
			{
				const unsigned char color_index= src[ x + y * int(pic.size[0]) ];
				if( color_index == 0 )
					continue;
				const uint32_t color= palette[ color_index ];

				for( int sy= 0; sy < int(menu_scale_); sy++ )
				{
					const int dst_y= start_y + y * int(menu_scale_) + sy;
					PC_ASSERT( dst_y >= 0 && dst_y < int(rendering_context_.viewport_size.Height()) );

					for( int sx= 0; sx < int(menu_scale_); sx++ )
					{
						const int dst_x= start_x + x * int(menu_scale_) + sx;
						PC_ASSERT( dst_x >= 0 && dst_x < int(rendering_context_.viewport_size.Width()) );
						dst_pixels[ dst_x + dst_y * dst_row_pixels ]= color;
					}
				}
			}
		}
	}
}

void MenuDrawerSoft::DrawConsoleBackground( const float console_pos )
{
	const PaletteTransformed& palette= *rendering_context_.palette_transformed;
	uint32_t* const dst_pixels= rendering_context_.window_surface_data;
	const int dst_row_pixels= rendering_context_.row_pixels;

	const Picture& pic= console_background_picture_;

	const int y_end= static_cast<int>( console_pos * float(rendering_context_.viewport_size.Height()) * 0.5f );
	const int y_start= int(rendering_context_.viewport_size.Height()) - y_end;

	for( int y= 0; y < y_end; y++ )
	{
		PC_ASSERT( y >= 0 && y < int(rendering_context_.viewport_size.Height()) );

		int tc_v= ( y + y_start ) / int( console_scale_ );
		if( tc_v < 0 ) tc_v= 0;
		if( tc_v >= int(pic.size[1]) ) tc_v= int(pic.size[1]) - 1;

		for( int x= 0; x < int(pic.size[0]); x++ )
		{
			const unsigned char color_index= pic.data[ tc_v * int(pic.size[0]) + x ];
			const uint32_t color= palette[ color_index ];

			for( int sx= 0; sx < int(console_scale_); sx++ )
			{
				const int dst_x= x * int(console_scale_) + sx;
				PC_ASSERT( dst_x >= 0 && dst_x < int(rendering_context_.viewport_size.Width()) );
				dst_pixels[ y * dst_row_pixels + dst_x ]= color;
			}
		}

		const int line_end_left= int( rendering_context_.viewport_size.Width() - console_scale_ * pic.size[0] );
		const uint32_t line_end_color= palette[ pic.data[ tc_v * int(pic.size[0]) + int(pic.size[0]) - 1 ] ];
		for( int i= 0; i < line_end_left; i++ )
			dst_pixels[ y * dst_row_pixels + int(pic.size[0]) * int(console_scale_) + i ]= line_end_color;
	}
}

void MenuDrawerSoft::DrawLoading( const float progress )
{
	const float progress_corrected= std::min( std::max( progress, 0.0f ), 1.0f );

	const PaletteTransformed& palette= *rendering_context_.palette_transformed;
	uint32_t* const dst_pixels= rendering_context_.window_surface_data;
	const int dst_row_pixels= rendering_context_.row_pixels;

	const Picture& pic= loading_picture_;

	const int start_x= int( ( rendering_context_.viewport_size.Width () - menu_scale_ * pic.size[0] ) / 2u );
	const int start_y= int( ( rendering_context_.viewport_size.Height() - menu_scale_ * ( pic.size[1] / 2u ) ) / 2u );
	const int progress_x= int( progress_corrected * float( pic.size[0] ) );

	for( int y= 0; y < int(pic.size[1] / 2u); y++ )
	{
		for( int i= 0; i < 2; i++ )
		{
			const int span_x_start= i == 0 ? 0 : progress_x;
			const int span_x_end= i == 0 ? progress_x : int(pic.size[0]);
			const int v_shift= i == 0 ? int(pic.size[1]/2u) : 0;
			for( int x= span_x_start; x < span_x_end; x++ )
			{
				const uint32_t color= palette[ pic.data[ x + ( y + v_shift ) * int(pic.size[0]) ] ];
				for( int sy= 0; sy < int(menu_scale_); sy++ )
				{
					const int dst_y= start_y + y * int(menu_scale_) + sy;
					PC_ASSERT( dst_y >= 0 && dst_y < int(rendering_context_.viewport_size.Height()) );
					for( int sx= 0; sx < int(menu_scale_); sx++ )
					{
						const int dst_x= start_x + x * int(menu_scale_) + sx;
						PC_ASSERT( dst_x >= 0 && dst_x < int(rendering_context_.viewport_size.Width()) );
						dst_pixels[ dst_y * dst_row_pixels + dst_x ]= color;
					}
				}
			}
		}
	}
}

void MenuDrawerSoft::DrawPaused()
{
	const PaletteTransformed& palette= *rendering_context_.palette_transformed;
	uint32_t* const dst_pixels= rendering_context_.window_surface_data;
	const int dst_row_pixels= rendering_context_.row_pixels;

	const Picture& pic= pause_picture_;

	const int start_x= int( ( rendering_context_.viewport_size.Width () - menu_scale_ * pic.size[0] ) / 2u );
	const int start_y= int( ( rendering_context_.viewport_size.Height() - menu_scale_ * pic.size[1] ) / 2u );

	for( int y= 0; y < int(pic.size[1]); y++ )
	for( int x= 0; x < int(pic.size[0]); x++ )
	{
		const uint32_t color= palette[ pic.data[ x + y * int(pic.size[0]) ] ];
		for( int sy= 0; sy < int(menu_scale_); sy++ )
		{
			const int dst_y= start_y + y * int(menu_scale_) + sy;
			PC_ASSERT( dst_y >= 0 && dst_y < int(rendering_context_.viewport_size.Height()) );
			for( int sx= 0; sx < int(menu_scale_); sx++ )
			{
				const int dst_x= start_x + x * int(menu_scale_) + sx;
				PC_ASSERT( dst_x >= 0 && dst_x < int(rendering_context_.viewport_size.Width()) );
				dst_pixels[ dst_y * dst_row_pixels + dst_x ]= color;
			}
		}
	}
}

void MenuDrawerSoft::DrawGameBackground()
{
	const PaletteTransformed& palette= *rendering_context_.palette_transformed;
	uint32_t* const dst_pixels= rendering_context_.window_surface_data;
	const int dst_row_pixels= rendering_context_.row_pixels;

	const int viewport_size_x= int(rendering_context_.viewport_size.Width ());
	const int viewport_size_y= int(rendering_context_.viewport_size.Height());

	const Picture& pic= game_background_picture_;

	unsigned int pic_size_scaled_x= pic.size[0] * menu_scale_;
	unsigned int pic_size_scaled_y= pic.size[1] * menu_scale_;
	const unsigned int tiles_x= ( rendering_context_.viewport_size.Width () + pic_size_scaled_x - 1u ) / pic_size_scaled_x;
	const unsigned int tiles_y= ( rendering_context_.viewport_size.Height() + pic_size_scaled_y - 1u ) / pic_size_scaled_y;

	for( int tile_y= 0; tile_y <= int(tiles_y); tile_y++ )
	for( int tile_x= 0; tile_x <= int(tiles_x); tile_x++ )
	{
		const int x_start= tile_x * int(pic_size_scaled_x);
		const int y_start= tile_y * int(pic_size_scaled_y);

		if( x_start + int( pic.size[0] * menu_scale_ ) <= viewport_size_x &&
			y_start + int( pic.size[1] * menu_scale_ ) <= viewport_size_y )
		{
			// Tile fully on screen
			for( int y= 0; y < int(pic.size[1]); y++ )
			for( int x= 0; x < int(pic.size[0]); x++ )
			{
				const uint32_t color= palette[ pic.data[ x + y * int(pic.size[0]) ] ];
				for( int sy= 0; sy < int(menu_scale_); sy++ )
				{
					const int dst_y= y_start + y * int(menu_scale_) + sy;
					PC_ASSERT( dst_y >= 0 && dst_y < int(rendering_context_.viewport_size.Height()) );
					for( int sx= 0; sx < int(menu_scale_); sx++ )
					{
						const int dst_x= x_start + x * int(menu_scale_) + sx;
						PC_ASSERT( dst_x >= 0 && dst_x < int(rendering_context_.viewport_size.Width()) );
						dst_pixels[ dst_x + dst_y * dst_row_pixels ]= color;
					}
				}
			}
		}
		else
		{
			// Tile on screen edge.
			for( int y= 0; y < int(pic.size[1]); y++ )
			for( int x= 0; x < int(pic.size[0]); x++ )
			{
				const uint32_t color= palette[ pic.data[ x + y * int(pic.size[0]) ] ];
				for( int sy= 0; sy < int(menu_scale_); sy++ )
				{
					const int dst_y= y_start + y * int(menu_scale_) + sy;
					if( dst_y >= viewport_size_y ) continue;

					for( int sx= 0; sx < int(menu_scale_); sx++ )
					{
						const int dst_x= x_start + x * int(menu_scale_) + sx;
						if( dst_x >= viewport_size_x ) continue;

						dst_pixels[ dst_x + dst_y * dst_row_pixels ]= color;
					}
				}
			}
		}
	} // for tiles
}

void MenuDrawerSoft::DrawBriefBar()
{
	const PaletteTransformed& palette= *rendering_context_.palette_transformed;
	uint32_t* const dst_pixels= rendering_context_.window_surface_data;
	const int dst_row_pixels= rendering_context_.row_pixels;

	const int viewport_size_x= int(rendering_context_.viewport_size.Width ());
	const int viewport_size_y= int(rendering_context_.viewport_size.Height());

	PC_ASSERT( int( briefbar_picture_.size[0] * menu_scale_ ) <= viewport_size_x );

	const int y_start= viewport_size_y - int( briefbar_picture_.size[1] * menu_scale_ );

	for( int y= 0; y < int(briefbar_picture_.size[1]); y++ )
	{
		for( int x= 0; x < int(briefbar_picture_.size[0]); x++ )
		{
			const uint32_t color= palette[ briefbar_picture_.data[ x + y * int(briefbar_picture_.size[0]) ] ];

			for( int sy= 0; sy < int(menu_scale_); sy++ )
			{
				const int dst_y= y_start + y * int(menu_scale_) + sy;
				PC_ASSERT( dst_y >= 0 && dst_y < viewport_size_y );
				for( int sx= 0; sx < int(menu_scale_); sx++ )
				{
					const int dst_x= x * int(menu_scale_) + sx;
					PC_ASSERT( dst_x >= 0 && dst_x < viewport_size_x );
					dst_pixels[ dst_x + dst_y * dst_row_pixels ]= color;
				}
			}
		}

		// Pixels at right border.
		const uint32_t color_right= palette[ briefbar_picture_.data[ int(briefbar_picture_.size[0]-1u) + y * int(briefbar_picture_.size[0]) ] ];
		for( int x= int( briefbar_picture_.size[0] * menu_scale_); x < viewport_size_x; x++ )
		{
			PC_ASSERT( x >= 0 && x < viewport_size_x );
			for( int sy= 0; sy < int(menu_scale_); sy++ )
			{
				const int dst_y= y_start + y * int(menu_scale_) + sy;
				PC_ASSERT( dst_y >= 0 && dst_y < viewport_size_y );
				dst_pixels[ x + dst_y * dst_row_pixels ]= color_right;
			}
		}
	} // for y
}

void MenuDrawerSoft::DrawPlayerTorso(
	const int x_start, const int y_start,
	const unsigned char color )
{
	const PaletteTransformed& palette= *rendering_context_.palette_transformed;
	uint32_t* const dst_pixels= rendering_context_.window_surface_data;
	const int dst_row_pixels= rendering_context_.row_pixels;

	const int viewport_size_x= int(rendering_context_.viewport_size.Width ());
	const int viewport_size_y= int(rendering_context_.viewport_size.Height());
	PC_UNUSED(viewport_size_x);
	PC_UNUSED(viewport_size_y);

	const int color_shift= GameConstants::player_colors_shifts[ color % GameConstants::player_colors_count ];

	for( int y= 0; y < int(player_torso_picutre_.size[1]); y++ )
	{
		for( int x= 0; x < int(player_torso_picutre_.size[0]); x++ )
		{
			int color_index= player_torso_picutre_.data[ x + y * int(player_torso_picutre_.size[0]) ];
			if( color_index == 255 )
				continue;

			if( color_index >= 14 * 16 && color_index < 14 * 16 + 16 )
				color_index+= color_shift;

			const uint32_t color= palette[ color_index ];

			for( int sy= 0; sy < int(menu_scale_); sy++ )
			{
				const int dst_y= y_start + y * int(menu_scale_) + sy;
				PC_ASSERT( dst_y >= 0 && dst_y < viewport_size_y );
				for( int sx= 0; sx < int(menu_scale_); sx++ )
				{
					const int dst_x= x_start + x * int(menu_scale_) + sx;
					PC_ASSERT( dst_x >= 0 && dst_x < viewport_size_x );
					dst_pixels[ dst_x + dst_y * dst_row_pixels ]= color;
				}
			}
		}
	}
}

} // namespace PanzerChas
