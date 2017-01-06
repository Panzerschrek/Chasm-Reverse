#include <cstring>

#include <ogl_state_manager.hpp>
#include <shaders_loading.hpp>

#include "../assert.hpp"
#include "../drawers.hpp"
#include "../game_resources.hpp"
#include "../images.hpp"

#include "hud_drawer.hpp"

namespace PanzerChasm
{

static constexpr unsigned int g_max_hud_quads= 64u;

static const GLenum g_gl_state_blend_func[2]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };

static const r_OGLState g_crosshair_gl_state(
	true, false, false, false,
	g_gl_state_blend_func );

static const r_OGLState g_hud_gl_state(
	false, false, false, false,
	g_gl_state_blend_func );

static const unsigned int g_hud_line_height= 32u;
static const unsigned int g_net_hud_line_height= 12u;

constexpr unsigned int g_cross_offset= 4u;

static void GenCrosshairTexture( const Palette& palette, r_Texture& out_texture )
{
	constexpr unsigned int c_size[2]= { 17u, 17u };
	constexpr unsigned int c_center[2]= { c_size[0] / 2u, c_size[1] / 2u };
	constexpr unsigned char c_start_color_index= 159u;

	unsigned char data_rgba[ 4u * c_size[0] * c_size[1] ]= { 0u };

	const auto set_pixel=
	[&]( const unsigned int x, const unsigned int y, const unsigned char color_index )
	{
		unsigned char* const dst= data_rgba + 4u * ( x + y * c_size[0] );
		dst[0]= palette[ 3u * color_index + 0u ];
		dst[1]= palette[ 3u * color_index + 1u ];
		dst[2]= palette[ 3u * color_index + 2u ];
		dst[3]= 255u;
	};

	for( unsigned int i= 0u; i < 4u; i++ )
	{
		const unsigned char color_index= c_start_color_index - i;
		set_pixel( c_center[0], c_center[1] + g_cross_offset + i, color_index );
		set_pixel( c_center[0], c_center[1] - g_cross_offset - i, color_index );
		set_pixel( c_center[0] + g_cross_offset + i, c_center[1], color_index );
		set_pixel( c_center[0] - g_cross_offset - i, c_center[1], color_index );
	}

	out_texture= r_Texture( r_Texture::PixelFormat::RGBA8, c_size[0], c_size[1], data_rgba );
	out_texture.SetFiltration( r_Texture::Filtration::Nearest, r_Texture::Filtration::Nearest );
}

static unsigned int CalculateHudScale( const Size2& viewport_size )
{
	// Status bu must have size 10% of screen height or smaller.
	const float c_y_relative_statusbar_size= 1.0f / 10.0f;

	float scale_f=
		std::min(
			float( viewport_size.Width () ) / float( GameConstants::min_screen_width  ),
			float( viewport_size.Height() ) / ( float(g_hud_line_height) / c_y_relative_statusbar_size ) );

	return std::max( 1u, static_cast<unsigned int>( scale_f ) );
}

HudDrawer::HudDrawer(
	const GameResourcesConstPtr& game_resources,
	const RenderingContext& rendering_context,
	const DrawersPtr& drawers )
	: game_resources_(game_resources)
	, viewport_size_( rendering_context.viewport_size )
	, drawers_(drawers)
	, scale_( CalculateHudScale( rendering_context.viewport_size ) )
{
	PC_ASSERT( game_resources_ != nullptr );
	PC_ASSERT( drawers_ != nullptr );

	std::memset( &player_state_, 0u, sizeof(player_state_) );

	// Textures
	GenCrosshairTexture( game_resources->palette, crosshair_texture_ );
	LoadTexture( "WICONS.CEL", 180u, weapon_icons_texture_ );
	LoadTexture( "BFONT2.CEL", 0u, hud_numbers_texture_ );
	LoadTexture( "STATUS2.CEL", 0u, hud_background_texture_ );

	{ // Vertex buffer
		unsigned short  indeces[ g_max_hud_quads * 6u ];
		for( unsigned int i= 0u; i < g_max_hud_quads; i++ )
		{
			unsigned short* const ind= indeces + 6u * i;
			ind[0]= 4u * i + 0u;  ind[1]= 4u * i + 1u;  ind[2]= 4u * i + 2u;
			ind[3]= 4u * i + 0u;  ind[4]= 4u * i + 2u;  ind[5]= 4u * i + 3u;
		}

		quad_buffer_.VertexData(
			nullptr,
			g_max_hud_quads * 4u * sizeof(Vertex),
			sizeof(Vertex) );

		quad_buffer_.IndexData(
			indeces,
			sizeof(indeces),
			GL_UNSIGNED_SHORT,
			GL_TRIANGLES );

		Vertex v;
		quad_buffer_.VertexAttribPointer(
			0,
			2, GL_SHORT, false,
			((char*)v.xy) - (char*)&v );
		quad_buffer_.VertexAttribPointer(
			1,
			2, GL_SHORT, false,
			((char*)v.tex_coord) - (char*)&v );
	}

	hud_shader_.ShaderSource(
		rLoadShader( "hud_f.glsl", rendering_context.glsl_version ),
		rLoadShader( "hud_v.glsl", rendering_context.glsl_version ) );
	hud_shader_.SetAttribLocation( "pos", 0u );
	hud_shader_.SetAttribLocation( "tex_coord", 1u );
	hud_shader_.Create();
}

HudDrawer::~HudDrawer()
{}

void HudDrawer::AddMessage( const MapData::Message& message, const Time current_time )
{
	current_message_= message;
	current_message_time_= current_time;
}

void HudDrawer::ResetMessage()
{
	current_message_time_= Time::FromSeconds(0);
}

void HudDrawer::SetPlayerState( const Messages::PlayerState& player_state, const unsigned int current_weapon_number )
{
	player_state_= player_state;
	current_weapon_number_= current_weapon_number;
}

void HudDrawer::DrawCrosshair()
{
	Vertex vertices[ g_max_hud_quads * 4u ];
	Vertex* v= vertices;

	const Size2 viewport_center( viewport_size_.xy[0] >> 1u, viewport_size_.xy[1] >> 1u );

	// In original game real view center shifted to upper crosshair bar start.
	const int y_shift= int(scale_) * g_cross_offset;

	v[0].xy[0]= int( viewport_center.xy[0] - scale_ * crosshair_texture_.Width () / 2u );
	v[0].xy[1]= int( viewport_center.xy[1] - scale_ * crosshair_texture_.Height() / 2u ) - y_shift;
	v[0].tex_coord[0]= 0;
	v[0].tex_coord[1]= 0;

	v[1].xy[0]= v[0].xy[0] + int( scale_ * crosshair_texture_.Width () );
	v[1].xy[1]= v[0].xy[1];
	v[1].tex_coord[0]= int( crosshair_texture_.Width () );
	v[1].tex_coord[1]= 0;

	v[2].xy[0]= v[0].xy[0] + int( scale_ * crosshair_texture_.Width () );
	v[2].xy[1]= v[0].xy[1] + int( scale_ * crosshair_texture_.Height() );
	v[2].tex_coord[0]= int( crosshair_texture_.Width () );
	v[2].tex_coord[1]= int( crosshair_texture_.Height() );

	v[3].xy[0]= v[0].xy[0];
	v[3].xy[1]= v[0].xy[1] + int( scale_ * crosshair_texture_.Height() );
	v[3].tex_coord[0]= 0;
	v[3].tex_coord[1]= int( crosshair_texture_.Height() );
	v+= 4u;

	const unsigned int vertex_count= ( v - vertices );
	const unsigned int index_count= vertex_count / 4u * 6u;

	quad_buffer_.VertexSubData( vertices, vertex_count * sizeof(Vertex), 0u );

	// Draw
	r_OGLStateManager::UpdateState( g_crosshair_gl_state );
	hud_shader_.Bind();

	crosshair_texture_.Bind(0u);
	hud_shader_.Uniform( "tex", int(0) );

	hud_shader_.Uniform(
		"inv_viewport_size",
		m_Vec2( 1.0f / float(viewport_size_.xy[0]), 1.0f / float(viewport_size_.xy[1]) ) );

	hud_shader_.Uniform(
		"inv_texture_size",
		 m_Vec2(
			1.0f / float(crosshair_texture_.Width ()),
			1.0f / float(crosshair_texture_.Height()) ) );

	glDrawElements( GL_TRIANGLES, index_count, GL_UNSIGNED_SHORT, nullptr );
}

void HudDrawer::DrawCurrentMessage( const Time current_time )
{
	if( current_message_time_.ToSeconds() == 0.0f )
		return;

	const float time_left= ( current_time - current_message_time_ ).ToSeconds();
	if( time_left > current_message_.delay_s )
		return;

	for( const  MapData::Message::Text& text : current_message_.texts )
	{
		int x;
		TextDraw::Alignment alignemnt;
		if( text.x == -1 )
		{
			x= drawers_->menu.GetViewportSize().Width() / 2u;
			alignemnt= TextDraw::Alignment::Center;
		}
		else
		{
			x= text.x * scale_;
			alignemnt= TextDraw::Alignment::Left;
		}

		drawers_->text.Print(
			x, text.y * scale_,
			text.data.c_str(),
			scale_,
			TextDraw::FontColor::YellowGreen, alignemnt );
	}
}

void HudDrawer::DrawHud( const bool draw_second_hud )
{
	Vertex vertices[ g_max_hud_quads * 4u ];
	Vertex* v= vertices;

	const unsigned int first_hud_bg_quad= ( v - vertices ) / 4u;

	const unsigned int hud_x= viewport_size_.Width() / 2u - hud_background_texture_.Width() * scale_ / 2u;

	{ // Hud background
		const unsigned int tc_y= g_net_hud_line_height + ( draw_second_hud ? g_hud_line_height : 0u );

		v[0].xy[0]= hud_x;
		v[0].xy[1]= 0;
		v[0].tex_coord[0]= 0;
		v[0].tex_coord[1]= tc_y + g_hud_line_height;

		v[1].xy[0]= hud_x + hud_background_texture_.Width() * scale_;
		v[1].xy[1]= 0;
		v[1].tex_coord[0]= hud_background_texture_.Width();
		v[1].tex_coord[1]= tc_y + g_hud_line_height;

		v[2].xy[0]= hud_x + hud_background_texture_.Width() * scale_;
		v[2].xy[1]= g_hud_line_height * scale_;
		v[2].tex_coord[0]= hud_background_texture_.Width();
		v[2].tex_coord[1]= tc_y;

		v[3].xy[0]= hud_x;
		v[3].xy[1]= g_hud_line_height * scale_;
		v[3].tex_coord[0]= 0;
		v[3].tex_coord[1]= tc_y;
		v+= 4u;
	}

	const unsigned int hud_bg_quad_count= ( v - vertices ) / 4u - first_hud_bg_quad;

	const unsigned int weapon_icon_first_quad= ( v - vertices ) / 4u;
	if( !draw_second_hud ) // Weapon icon
	{
		const int c_border= 1;
		const int border= int(scale_) * c_border;
		const unsigned int icon_width = weapon_icons_texture_.Width() / 8u;
		const unsigned int icon_height= weapon_icons_texture_.Height();
		const unsigned int tc_x= current_weapon_number_ * icon_width;
		const unsigned int y= 4u * scale_;

		const unsigned int icon_x= hud_x + 105u * scale_;

		v[0].xy[0]= icon_x + border;
		v[0].xy[1]= y + border;
		v[0].tex_coord[0]= tc_x + border;
		v[0].tex_coord[1]= icon_height - border;

		v[1].xy[0]= icon_x + icon_width * scale_ - border;
		v[1].xy[1]= y + border;
		v[1].tex_coord[0]= tc_x + icon_width - border;
		v[1].tex_coord[1]= icon_height - border;

		v[2].xy[0]= icon_x + icon_width * scale_ - border;
		v[2].xy[1]= y + icon_height * scale_ - border;
		v[2].tex_coord[0]= tc_x + icon_width - border;
		v[2].tex_coord[1]= border;

		v[3].xy[0]= icon_x + border;
		v[3].xy[1]= y + icon_height * scale_ - border;
		v[3].tex_coord[0]= tc_x + border;
		v[3].tex_coord[1]= border;
		v+= 4u;
	}
	const unsigned int weapon_icon_quad_count= ( v - vertices ) / 4u - weapon_icon_first_quad;

	const auto gen_number=
	[&]( const unsigned int x_end, const unsigned int number, const unsigned int red_value )
	{
		char digits[8];
		std::snprintf( digits, sizeof(digits), "%d", number );
		const unsigned int digit_count= std::strlen(digits);

		const unsigned int digit_width = hud_numbers_texture_.Width() / 10u;
		const unsigned int digit_height= ( hud_numbers_texture_.Height() - 1u ) / 2u;
		const unsigned int y= ( g_hud_line_height - digit_height ) / 2u * scale_;
		const unsigned int tc_y= number >= red_value ? 0u : digit_height + 1u ;

		for( unsigned int d= 0u; d < digit_count; d++ )
		{
			const unsigned int tc_x= ( digits[d] - '0' ) * digit_width;
			const unsigned int digit_x= x_end - scale_ * digit_width * ( digit_count - d );

			v[0].xy[0]= digit_x;
			v[0].xy[1]= y;
			v[0].tex_coord[0]= tc_x;
			v[0].tex_coord[1]= tc_y + digit_height;

			v[1].xy[0]= digit_x + ( digit_width - 1u ) * scale_;
			v[1].xy[1]= y;
			v[1].tex_coord[0]= tc_x + digit_width - 1u;
			v[1].tex_coord[1]= tc_y + digit_height;

			v[2].xy[0]= digit_x + ( digit_width - 1u ) * scale_;
			v[2].xy[1]= y + digit_height * scale_;
			v[2].tex_coord[0]= tc_x + digit_width - 1u;
			v[2].tex_coord[1]= tc_y;

			v[3].xy[0]= digit_x;
			v[3].xy[1]= y + digit_height * scale_;
			v[3].tex_coord[0]= tc_x;
			v[3].tex_coord[1]= tc_y;
			v+= 4u;
		}
	};

	const unsigned int numbers_first_quad= ( v - vertices ) / 4u;

	gen_number( hud_x + scale_ * 104u, player_state_.health, 25u );
	if( !draw_second_hud )
	{
		if( current_weapon_number_ != 0u )
			gen_number( hud_x + scale_ * 205u, player_state_.ammo[ current_weapon_number_ ], 10u );
		gen_number( hud_x + scale_ * 315u, player_state_.armor, 10u );
	}

	const unsigned int numbers_quad_count= ( v - vertices ) / 4u - numbers_first_quad;

	quad_buffer_.VertexSubData( vertices, ( v - vertices ) * sizeof(Vertex), 0u );

	// Draw
	r_OGLStateManager::UpdateState( g_hud_gl_state );
	hud_shader_.Bind();
	hud_shader_.Uniform( "tex", int(0) );
	hud_shader_.Uniform(
		"inv_viewport_size",
		m_Vec2( 1.0f / float(viewport_size_.xy[0]), 1.0f / float(viewport_size_.xy[1]) ) );

	const auto draw_quads=
	[&]( const r_Texture& texture, const unsigned int first_quad, const unsigned int quad_count )
	{
		if( quad_count == 0u )
			return;

		texture.Bind(0u);
		hud_shader_.Uniform(
			"inv_texture_size",
			 m_Vec2(
				1.0f / float(texture.Width ()),
				1.0f / float(texture.Height()) ) );

		glDrawElements(
			GL_TRIANGLES,
			quad_count * 6u,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>( first_quad * 6u * sizeof(unsigned short) ) );
	};

	draw_quads( hud_background_texture_, first_hud_bg_quad, hud_bg_quad_count );
	draw_quads( weapon_icons_texture_, weapon_icon_first_quad, weapon_icon_quad_count );
	draw_quads( hud_numbers_texture_, numbers_first_quad, numbers_quad_count );

	// Keys
	if( draw_second_hud )
	{
		const unsigned int c_left_x= 113u;
		const unsigned int c_right_x= 133u;
		const unsigned int c_top_y= 28u;
		const unsigned int c_bottom_y= 14u;

		if( ( player_state_.keys_mask & 1u ) != 0u )
			drawers_->text.Print( hud_x + scale_ * c_left_x , viewport_size_.Height() - c_top_y    * scale_, "\4", scale_ );
		if( ( player_state_.keys_mask & 2u ) != 0u )
			drawers_->text.Print( hud_x + scale_ * c_right_x, viewport_size_.Height() - c_top_y    * scale_, "\5", scale_ );
		if( ( player_state_.keys_mask & 4u ) != 0u )
			drawers_->text.Print( hud_x + scale_ * c_left_x , viewport_size_.Height() - c_bottom_y * scale_, "\6", scale_ );
		if( ( player_state_.keys_mask & 8u ) != 0u )
			drawers_->text.Print( hud_x + scale_ * c_right_x, viewport_size_.Height() - c_bottom_y * scale_, "\7", scale_ );
	}
}

void HudDrawer::LoadTexture(
	const char* const file_name,
	const unsigned char alpha_color_index,
	r_Texture& out_texture )
{
	const Vfs::FileContent texture_file= game_resources_->vfs->ReadFile( file_name );
	const CelTextureHeader& cel= *reinterpret_cast<const CelTextureHeader*>( texture_file.data() );

	const unsigned char* const texture_data= texture_file.data() + sizeof(CelTextureHeader);
	const unsigned int pixel_count= cel.size[0] * cel.size[1];
	std::vector<unsigned char> texture_data_rgba( 4u * pixel_count );
	ConvertToRGBA( pixel_count, texture_data, game_resources_->palette, texture_data_rgba.data(), alpha_color_index );

	out_texture=
		r_Texture(
			r_Texture::PixelFormat::RGBA8,
			cel.size[0], cel.size[1],
			texture_data_rgba.data() );

	out_texture.SetFiltration( r_Texture::Filtration::Nearest, r_Texture::Filtration::Nearest );
}

} // namespace PanzerChasm
