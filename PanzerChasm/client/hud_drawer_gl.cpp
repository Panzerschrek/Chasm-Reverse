#include <cstring>

#include <ogl_state_manager.hpp>
#include <shaders_loading.hpp>

#include "../assert.hpp"
#include "../game_resources.hpp"
#include "../images.hpp"
#include "../i_menu_drawer.hpp"
#include "../i_text_drawer.hpp"
#include "../shared_drawers.hpp"

#include "hud_drawer_gl.hpp"

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

HudDrawerGL::HudDrawerGL(
	const GameResourcesConstPtr& game_resources,
	const RenderingContextGL& rendering_context,
	const SharedDrawersPtr& shared_drawers )
	: HudDrawerBase( game_resources, shared_drawers )
	, viewport_size_( rendering_context.viewport_size )
{
	// Textures
	{
		unsigned int size[2];
		std::vector<unsigned char> data, data_rgba;
		CreateCrosshairTexture( size, data );
		data_rgba.resize( data.size() * 4u );
		ConvertToRGBA( size[0] * size[1], data.data(), game_resources_->palette, data_rgba.data(), 0u );

		crosshair_texture_= r_Texture( r_Texture::PixelFormat::RGBA8, size[0], size[1], data_rgba.data() );
		crosshair_texture_.SetFiltration( r_Texture::Filtration::Nearest, r_Texture::Filtration::Nearest );
	}

	LoadTexture( c_weapon_icons_image_file_name, 180u, weapon_icons_texture_ );
	LoadTexture( c_hud_numbers_image_file_name, 0u, hud_numbers_texture_ );
	LoadTexture( c_hud_background_image_file_name, 0u, hud_background_texture_ );

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

HudDrawerGL::~HudDrawerGL()
{}

void HudDrawerGL::DrawCrosshair()
{
	Vertex vertices[ g_max_hud_quads * 4u ];
	Vertex* v= vertices;

	const Size2 viewport_center( viewport_size_.xy[0] >> 1u, viewport_size_.xy[1] >> 1u );

	// In original game real view center shifted to upper crosshair bar start.
	const int y_shift= int(scale_) * c_cross_offset;

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

void HudDrawerGL::DrawHud( const bool draw_second_hud, const char* const map_name )
{
	Vertex vertices[ g_max_hud_quads * 4u ];
	Vertex* v= vertices;

	const unsigned int first_hud_bg_quad= ( v - vertices ) / 4u;

	const unsigned int hud_x= viewport_size_.Width() / 2u - hud_background_texture_.Width() * scale_ / 2u;

	{ // Hud background
		const unsigned int tc_y= c_net_hud_line_height + ( draw_second_hud ? c_hud_line_height : 0u );

		v[0].xy[0]= hud_x;
		v[0].xy[1]= 0;
		v[0].tex_coord[0]= 0;
		v[0].tex_coord[1]= tc_y + c_hud_line_height;

		v[1].xy[0]= hud_x + hud_background_texture_.Width() * scale_;
		v[1].xy[1]= 0;
		v[1].tex_coord[0]= hud_background_texture_.Width();
		v[1].tex_coord[1]= tc_y + c_hud_line_height;

		v[2].xy[0]= hud_x + hud_background_texture_.Width() * scale_;
		v[2].xy[1]= c_hud_line_height * scale_;
		v[2].tex_coord[0]= hud_background_texture_.Width();
		v[2].tex_coord[1]= tc_y;

		v[3].xy[0]= hud_x;
		v[3].xy[1]= c_hud_line_height * scale_;
		v[3].tex_coord[0]= 0;
		v[3].tex_coord[1]= tc_y;
		v+= 4u;
	}

	const unsigned int hud_bg_quad_count= ( v - vertices ) / 4u - first_hud_bg_quad;

	const unsigned int weapon_icon_first_quad= ( v - vertices ) / 4u;
	if( !draw_second_hud ) // Weapon icon
	{
		const int c_border= c_weapon_icon_border;
		const int border= int(scale_) * c_border;
		const unsigned int icon_width = weapon_icons_texture_.Width() / 8u;
		const unsigned int icon_height= weapon_icons_texture_.Height();
		const unsigned int tc_x= current_weapon_number_ * icon_width;
		const unsigned int y= c_weapon_icon_y_offset * scale_;

		const unsigned int icon_x= hud_x + c_weapon_icon_x_offset * scale_;

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
		const unsigned int y= ( c_hud_line_height - digit_height ) / 2u * scale_;
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

	gen_number( hud_x + scale_ * c_health_x_offset, player_state_.health, c_health_red_value );
	if( !draw_second_hud )
	{
		if( current_weapon_number_ != 0u )
			gen_number( hud_x + scale_ * c_ammo_x_offset, player_state_.ammo[ current_weapon_number_ ], c_ammo_red_value );
		gen_number( hud_x + scale_ * c_armor_x_offset, player_state_.armor, c_armor_red_value );
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

	if( draw_second_hud )
		HudDrawerBase::DrawKeysAndStat( hud_x, map_name );
}

void HudDrawerGL::LoadTexture(
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
