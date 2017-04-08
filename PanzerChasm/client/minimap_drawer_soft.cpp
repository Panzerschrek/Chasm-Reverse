#include <matrix.hpp>

#include "../assert.hpp"
#include "../map_loader.hpp"
#include "../math_utils.hpp"
#include "minimap_drawers_common.hpp"

#include "minimap_drawer_soft.hpp"

namespace PanzerChasm
{

MinimapDrawerSoft::MinimapDrawerSoft(
	Settings& settings,
	const GameResourcesConstPtr& game_resources,
	const RenderingContextSoft& rendering_context )
	: rendering_context_(rendering_context)
	, game_resources_(game_resources)
{
	PC_ASSERT( game_resources_ != nullptr );

	// TODO
	PC_UNUSED( settings );
}

MinimapDrawerSoft::~MinimapDrawerSoft()
{}

void MinimapDrawerSoft::SetMap( const MapDataConstPtr& map_data )
{
	current_map_data_= map_data;
}

void MinimapDrawerSoft::Draw(
	const MapState& map_state, const MinimapState& minimap_state,
	const m_Vec2& camera_position, const float view_angle )
{
	if( current_map_data_ == nullptr )
		return;

	const PaletteTransformed& palette= *rendering_context_.palette_transformed;

	const unsigned int scale_i= CalculateMinimapScale( rendering_context_.viewport_size );
	const float scale= static_cast<float>( scale_i );

	line_width_= scale;

	const int top= static_cast<int>( MinimapParams::top_offset_px * scale );
	const int left= static_cast<int>( MinimapParams::left_offset_px * scale );
	const int right= int(rendering_context_.viewport_size.Width()) / 2 - static_cast<int>( MinimapParams::right_offset_from_center_px * scale );
	const int bottom= static_cast<int>( ( 1.0f - MinimapParams::bottom_offset_rel ) * static_cast<float>( rendering_context_.viewport_size.Height() ) );
	const int map_viewport_width = right - left;
	const int map_viewport_height= bottom - top;

	x_min_= left  ; x_min_f_= x_min_ << 16;
	y_min_= top   ; y_min_f_= y_min_ << 16;
	x_max_= right ; x_max_f_= x_max_ << 16;
	y_max_= bottom; y_max_f_= y_max_ << 16;

	const float transform_x= float(map_viewport_width ) * 0.5f;
	const float transform_y= float(map_viewport_height) * 0.5f;
	const float ransform_add_x= float(x_min_) + transform_x;
	const float ransform_add_y= float(y_min_) + transform_y;

	m_Mat4 shift_mat, scale_mat, screen_scale_mat, screen_shift_mat, final_mat;
	shift_mat.Translate( -m_Vec3( camera_position, 0.0f ) );
	const float c_base_scale= -1.0f / 12.0f;
	scale_mat.Scale( c_base_scale );
	const float aspect= float(map_viewport_height) / float(map_viewport_width);
	screen_scale_mat.Scale( m_Vec3( transform_x * aspect, -transform_y, 0.0f ) );
	screen_shift_mat.Translate( m_Vec3( ransform_add_x, ransform_add_y, 0.0f ) );
	final_mat= shift_mat * scale_mat * screen_scale_mat * screen_shift_mat;

	// Use 3x3 matrix for 2d space. It is faster, obviously.
	m_Mat3 final_mat3( final_mat );
	final_mat3.value[6]= final_mat.value[12];
	final_mat3.value[7]= final_mat.value[13];

	const MinimapState::WallsVisibility& static_walls_visibility = minimap_state.GetStaticWallsVisibility ();
	const MinimapState::WallsVisibility& dynamic_walls_visibility= minimap_state.GetDynamicWallsVisibility();

	line_color_= palette[ MinimapParams::walls_color ];
	for( unsigned int w= 0u; w < current_map_data_->static_walls.size(); w++ )
	{
		if( !static_walls_visibility[w] )
			continue;

		const MapData::Wall& wall= current_map_data_->static_walls[w];
		const m_Vec2 v0= wall.vert_pos[0] * final_mat3;
		const m_Vec2 v1= wall.vert_pos[1] * final_mat3;

		DrawLine(
			fixed16_t( v0.x * 65536.0f ), fixed16_t( v0.y * 65536.0f ),
			fixed16_t( v1.x * 65536.0f ), fixed16_t( v1.y * 65536.0f ) );
	}

	const MapState::DynamicWalls& dynamic_walls= map_state.GetDynamicWalls();
	for( unsigned int w= 0u; w < dynamic_walls.size(); w++ )
	{
		if( !dynamic_walls_visibility[w] )
			continue;

		const MapState::DynamicWall& wall= dynamic_walls[w];
		const m_Vec2 v0= wall.vert_pos[0] * final_mat3;
		const m_Vec2 v1= wall.vert_pos[1] * final_mat3;

		DrawLine(
			fixed16_t( v0.x * 65536.0f ), fixed16_t( v0.y * 65536.0f ),
			fixed16_t( v1.x * 65536.0f ), fixed16_t( v1.y * 65536.0f ) );
	}

	{ // Arrow
		m_Mat4 arrow_rotate_mat, arrow_final_mat;
		arrow_rotate_mat.RotateZ( view_angle );
		arrow_final_mat= arrow_rotate_mat * scale_mat * screen_scale_mat * screen_shift_mat;

		static m_Vec2 c_arrow_vertices[]=
			{ { -0.25f, -0.5f }, { 0.25f, -0.5f }, { 0.0f, 0.5f } };

		line_color_= palette[ MinimapParams::arrow_color ];

		for( unsigned int i= 0u; i < sizeof(c_arrow_vertices)/ sizeof(c_arrow_vertices[0]); i++ )
		{
			unsigned int prev_i= i == 0u ? sizeof(c_arrow_vertices)/ sizeof(c_arrow_vertices[0]) - 1u : i - 1u;

			m_Vec2 v0= ( m_Vec3( c_arrow_vertices[i     ], 0.0f ) * arrow_final_mat ).xy();
			m_Vec2 v1= ( m_Vec3( c_arrow_vertices[prev_i], 0.0f ) * arrow_final_mat ).xy();

			DrawLine(
				fixed16_t( v0.x * 65536.0f ), fixed16_t( v0.y * 65536.0f ),
				fixed16_t( v1.x * 65536.0f ), fixed16_t( v1.y * 65536.0f ) );
		}
	}

	x_min_= 0;
	y_min_= 0;
	x_max_= rendering_context_.viewport_size.Width ();
	y_max_= rendering_context_.viewport_size.Height();
	x_min_f_= x_min_ << 16;
	y_min_f_= y_min_ << 16;
	x_max_f_= x_max_ << 16;
	y_max_f_= y_max_ << 16;

	// Framing

	line_color_= palette[ MinimapParams::framing_contour_color ];
	line_width_= scale_i * 3;
	DrawLine( left  << 16, top    << 16, right << 16, top    << 16 );
	DrawLine( left  << 16, top    << 16, left  << 16, bottom << 16 );
	DrawLine( right << 16, top    << 16, right << 16, bottom << 16 );
	DrawLine( left  << 16, bottom << 16, right << 16, bottom << 16 );

	line_color_= palette[ MinimapParams::framing_color ];
	line_width_= scale_i;
	DrawLine( left  << 16, top    << 16, right << 16, top    << 16 );
	DrawLine( left  << 16, top    << 16, left  << 16, bottom << 16 );
	DrawLine( right << 16, top    << 16, right << 16, bottom << 16 );
	DrawLine( left  << 16, bottom << 16, right << 16, bottom << 16 );

}

void MinimapDrawerSoft::DrawLine(
	const fixed16_t x0, const fixed16_t y0,
	const fixed16_t x1, const fixed16_t y1 )
{
	// Do fast line reject.
	if( x0 <  x_min_f_ && x1 <  x_min_f_ ) return;
	if( y0 <  y_min_f_ && y1 <  y_min_f_ ) return;
	if( x0 >= x_max_f_ && x1 >= x_max_f_ ) return;
	if( y0 >= y_max_f_ && y1 >= y_min_f_ ) return;

	uint32_t* const dst_pixels= rendering_context_.window_surface_data;
	const int dst_row_pixels= rendering_context_.row_pixels;

	const int width= line_width_;
	const int width_minus_delta= width / 2;
	const int width_delta= width - width_minus_delta;

	const fixed16_t dx= x1 - x0;
	const fixed16_t dy= y1 - y0;
	const fixed16_t abs_dx= std::abs( dx );
	const fixed16_t abs_dy= std::abs( dy );

	if( dx == 0 && dy == 0 )
		return;

	if( abs_dx > abs_dy )
	{
		fixed16_t dy_dx;
		fixed16_t x_start, x_end, y_start;
		if( dx > 0 )
		{
			dy_dx= Fixed16Div( dy, dx );
			x_start= x0;
			x_end= x1;
			y_start= y0;
		}
		else
		{
			dy_dx= Fixed16Div( -dy, -dx );
			x_start= x1;
			x_end= x0;
			y_start= y1;
		}

		const int x_start_i= std::max( Fixed16RoundToInt( x_start ), x_min_ );
		const int x_end_i  = std::min( Fixed16RoundToInt( x_end   ), x_max_ );

		const fixed16_t x_cut= ( x_start_i << 16 ) + g_fixed16_half - x_start;
		fixed16_t y= y_start + Fixed16Mul( dy_dx, x_cut );

		for( int x= x_start_i; x < x_end_i; x++, y+= dy_dx )
		{
			const int y_i= Fixed16RoundToInt(y);
			for( int y= y_i - width_minus_delta; y < y_i + width_delta; y++ )
			{
				if( y < y_min_ || y >= y_max_ ) continue;
				dst_pixels[ x + y * dst_row_pixels ]= line_color_;
			}
		}
	}
	else
	{
		fixed16_t dx_dy;
		fixed16_t y_start, y_end, x_start;
		if( dy > 0 )
		{
			dx_dy= Fixed16Div( dx, dy );
			y_start= y0;
			y_end= y1;
			x_start= x0;
		}
		else
		{
			dx_dy= Fixed16Div( -dx, -dy );
			y_start= y1;
			y_end= y0;
			x_start= x1;
		}

		const int y_start_i= std::max( Fixed16RoundToInt( y_start ), y_min_ );
		const int y_end_i  = std::min( Fixed16RoundToInt( y_end   ), y_max_ );

		const fixed16_t y_cut= ( y_start_i << 16 ) + g_fixed16_half - y_start;
		fixed16_t x= x_start + Fixed16Mul( dx_dy, y_cut );

		for( int y= y_start_i; y < y_end_i; y++, x+= dx_dy )
		{
			const int x_i= Fixed16RoundToInt(x);
			for( int x= x_i - width_minus_delta; x < x_i + width_delta; x++ )
			{
				if( x < x_min_ || x >= x_max_ ) continue;
				dst_pixels[ x + y * dst_row_pixels ]= line_color_;
			}
		}
	}
}

} // namespace PanzerChasm
