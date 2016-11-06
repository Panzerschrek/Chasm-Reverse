#include <ogl_state_manager.hpp>
#include <shaders_loading.hpp>

#include "assert.hpp"

#include "hud_drawer.hpp"

namespace PanzerChasm
{

static constexpr unsigned int g_max_hud_quads= 64u;

static const GLenum g_gl_state_blend_func[2]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
static const r_OGLState g_gl_state(
	true, false, false, false,
	g_gl_state_blend_func );

static void GenCrosshairTexture( const Palette& palette, r_Texture& out_texture )
{
	constexpr unsigned int c_size[2]= { 17u, 17u };
	constexpr unsigned int c_center[2]= { c_size[0] / 2u, c_size[1] / 2u };
	constexpr unsigned int c_cross_offset= 4u;
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
		set_pixel( c_center[0], c_center[1] + c_cross_offset + i, color_index );
		set_pixel( c_center[0], c_center[1] - c_cross_offset - i, color_index );
		set_pixel( c_center[0] + c_cross_offset + i, c_center[1], color_index );
		set_pixel( c_center[0] - c_cross_offset - i, c_center[1], color_index );
	}

	out_texture= r_Texture( r_Texture::PixelFormat::RGBA8, c_size[0], c_size[1], data_rgba );
	out_texture.SetFiltration( r_Texture::Filtration::Nearest, r_Texture::Filtration::Nearest );
}

HudDrawer::HudDrawer(
	const GameResourcesConstPtr& game_resources,
	const RenderingContext& rendering_context )
	: game_resources_(game_resources)
	, viewport_size_( rendering_context.viewport_size )
{
	PC_ASSERT( game_resources_ != nullptr );

	// Textures
	GenCrosshairTexture( game_resources->palette, crosshair_texture_ );

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

void HudDrawer::DrawCrosshair( const unsigned int scale )
{
	Vertex vertices[ g_max_hud_quads * 4u ];
	Vertex* v= vertices;

	const Size2 viewport_center( viewport_size_.xy[0] >> 1u, viewport_size_.xy[1] >> 1u );

	v[0].xy[0]= int( viewport_center.xy[0] - scale * crosshair_texture_.Width () / 2u );
	v[0].xy[1]= int( viewport_center.xy[1] - scale * crosshair_texture_.Height() / 2u );
	v[0].tex_coord[0]= 0;
	v[0].tex_coord[1]= 0;

	v[1].xy[0]= v[0].xy[0] + int( scale * crosshair_texture_.Width () );
	v[1].xy[1]= v[0].xy[1];
	v[1].tex_coord[0]= int( crosshair_texture_.Width () );
	v[1].tex_coord[1]= 0;

	v[2].xy[0]= v[0].xy[0] + int( scale * crosshair_texture_.Width () );
	v[2].xy[1]= v[0].xy[1] + int( scale * crosshair_texture_.Height() );
	v[2].tex_coord[0]= int( crosshair_texture_.Width () );
	v[2].tex_coord[1]= int( crosshair_texture_.Height() );

	v[3].xy[0]= v[0].xy[0];
	v[3].xy[1]= v[0].xy[1] + int( scale * crosshair_texture_.Height() );
	v[3].tex_coord[0]= 0;
	v[3].tex_coord[1]= int( crosshair_texture_.Height() );
	v+= 4u;

	const unsigned int vertex_count= ( v - vertices );
	const unsigned int index_count= vertex_count / 4u * 6u;

	quad_buffer_.VertexSubData( vertices, vertex_count * sizeof(Vertex), 0u );

	// Draw
	r_OGLStateManager::UpdateState( g_gl_state );
	hud_shader_.Bind();

	crosshair_texture_.Bind(0u);
	hud_shader_.Uniform( "tile_tex", int(0) );

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

} // namespace PanzerChasm
