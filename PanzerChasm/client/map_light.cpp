#include <ogl_state_manager.hpp>

#include "../map_loader.hpp"

#include "map_light.hpp"

namespace PanzerChasm
{

namespace
{

const unsigned int g_wall_lightmap_size= 32u;
const Size2 g_walls_lightmap_atlas_size(
	MapData::c_map_size * g_wall_lightmap_size,
	MapData::c_map_size );

const float g_lightmap_clear_color[4u]= { 0.0f, 0.0f, 0.0f, 0.0f };
const GLenum g_gl_state_blend_func[2]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };

const r_OGLState g_lightmap_clear_state(
	false, false, false, false,
	g_gl_state_blend_func,
	g_lightmap_clear_color );

const GLenum g_light_pass_blend_func[2]= { GL_ONE, GL_ONE };
const r_OGLState g_light_pass_state(
	true, false, false, false,
	g_light_pass_blend_func );

} // namespace


MapLight::MapLight( const RenderingContext& rendering_context )
{
	const unsigned int c_hd_lightmap_scale= 4u;

	// Floor
	base_floor_lightmap_=
		r_Framebuffer(
			{ r_Texture::PixelFormat::R8 }, r_Texture::PixelFormat::Unknown,
			c_hd_lightmap_scale * MapData::c_lightmap_size, c_hd_lightmap_scale * MapData::c_lightmap_size );
	final_floor_lightmap_=
		r_Framebuffer(
			{ r_Texture::PixelFormat::R8 }, r_Texture::PixelFormat::Unknown,
			c_hd_lightmap_scale * MapData::c_lightmap_size, c_hd_lightmap_scale * MapData::c_lightmap_size );

	// Walls
	base_walls_lightmap_=
		r_Framebuffer(
			{ r_Texture::PixelFormat::R8 }, r_Texture::PixelFormat::Unknown,
			g_walls_lightmap_atlas_size.Width(), g_walls_lightmap_atlas_size.Height() );
	final_walls_lightmap_=
		r_Framebuffer(
			{ r_Texture::PixelFormat::R8 }, r_Texture::PixelFormat::Unknown,
			g_walls_lightmap_atlas_size.Width(), g_walls_lightmap_atlas_size.Height() );

	// Shadowmap
	shadowmap_=
		r_Framebuffer(
			{ r_Texture::PixelFormat::R8 }, r_Texture::PixelFormat::Unknown,
			c_hd_lightmap_scale * MapData::c_lightmap_size, c_hd_lightmap_scale * MapData::c_lightmap_size );

	// Shaders
	floor_light_pass_shader_.ShaderSource(
		rLoadShader( "light_f.glsl", rendering_context.glsl_version ),
		rLoadShader( "light_v.glsl", rendering_context.glsl_version ) );
	floor_light_pass_shader_.Create();

	floor_ambient_light_pass_shader_.ShaderSource(
		rLoadShader( "ambient_light_f.glsl", rendering_context.glsl_version ),
		rLoadShader( "ambient_light_v.glsl", rendering_context.glsl_version ) );
	floor_ambient_light_pass_shader_.Create();

	shadowmap_shader_.ShaderSource(
		rLoadShader( "shadowmap_f.glsl", rendering_context.glsl_version ),
		rLoadShader( "shadowmap_v.glsl", rendering_context.glsl_version ),
		rLoadShader( "shadowmap_g.glsl", rendering_context.glsl_version ) );
	shadowmap_shader_.SetAttribLocation( "pos", 0u );
	shadowmap_shader_.Create();
}

MapLight::~MapLight()
{}

void MapLight::SetMap( const MapDataConstPtr& map_data )
{
	map_data_= map_data;

	// Occludders buffer.
	r_PolygonBuffer walls_buffer;
	{
		std::vector<short> vertices;

		for( const MapData::Wall& wall : map_data_->static_walls )
		{
			if( wall.texture_id >= 87u )
				continue;
			if( map_data_->walls_textures[ wall.texture_id ].file_path[0] == '\0' )
				continue;

			vertices.resize( vertices.size() + 4u );
			short* const v= vertices.data() + vertices.size() - 4u;

			v[0]= static_cast<short>( wall.vert_pos[0].x * 256.0f );
			v[1]= static_cast<short>( wall.vert_pos[0].y * 256.0f );
			v[2]= static_cast<short>( wall.vert_pos[1].x * 256.0f );
			v[3]= static_cast<short>( wall.vert_pos[1].y * 256.0f );
		}

		walls_buffer.VertexData( vertices.data(), vertices.size() * sizeof(short), sizeof(short) * 2u );
		walls_buffer.VertexAttribPointer( 0, 2, GL_SHORT, false, 0 );
		walls_buffer.SetPrimitiveType( GL_LINES );
	}

	base_floor_lightmap_.Bind();
	r_OGLStateManager::UpdateState( g_lightmap_clear_state );
	glClear( GL_COLOR_BUFFER_BIT );

	for( const MapData::Light& light : map_data_->lights )
	{
		if( light.power <= 1.0f )
			continue;

		// Bind and clear shadowmam.
		shadowmap_.Bind();
		r_OGLStateManager::UpdateState( g_lightmap_clear_state );
		glClear( GL_COLOR_BUFFER_BIT );

		// Draw walls into shadowmap.
		shadowmap_shader_.Bind();

		m_Mat4 shadow_scale_mat, shadow_shift_mat, shadow_view_mat, shadow_vertices_scale_mat;
		shadow_scale_mat.Scale( 2.0f / float(MapData::c_map_size) );
		shadow_shift_mat.Translate( m_Vec3( -1.0f, -1.0f, 0.0f ) );
		shadow_vertices_scale_mat.Scale( 1.0f / 256.0f );

		shadow_view_mat= shadow_scale_mat * shadow_shift_mat;

		shadowmap_shader_.Uniform( "view_matrix", shadow_vertices_scale_mat * shadow_view_mat );
		shadowmap_shader_.Uniform( "light_pos", ( m_Vec3( light.pos, 0.0f ) * shadow_view_mat ).xy() );

		const float shadowmap_texel_size= 2.0f / float( shadowmap_.GetTextures().front().Width() );
		shadowmap_shader_.Uniform( "offset", shadowmap_texel_size * 1.5f );

		// TODO - use scissor test for speed.
		walls_buffer.Draw();

		// Build lightmap.
		base_floor_lightmap_.Bind();
		r_OGLStateManager::UpdateState( g_light_pass_state );
		floor_light_pass_shader_.Bind();

		// Make light pass.
		shadowmap_.GetTextures().front().Bind(0);

		const float lightmap_texel_size= float( MapData::c_map_size ) / float( base_floor_lightmap_.Width() );
		const float half_map_size= 0.5f * float(MapData::c_map_size);
		const float extended_radius= light.outer_radius + lightmap_texel_size;

		m_Mat4 world_scale_mat, viewport_scale_mat, world_shift_mat, viewport_shift_mat, world_mat, viewport_mat;
		world_scale_mat.Scale( extended_radius );
		viewport_scale_mat.Scale( extended_radius / half_map_size );

		world_shift_mat.Translate( m_Vec3( light.pos, 0.0f ) );
		viewport_shift_mat.Translate(
			m_Vec3(
				( light.pos - m_Vec2( half_map_size, half_map_size ) ) / half_map_size,
				0.0f ) );

		world_mat= world_scale_mat * world_shift_mat;
		viewport_mat= viewport_scale_mat * viewport_shift_mat;

		floor_light_pass_shader_.Uniform( "shadowmap", int(0) );
		floor_light_pass_shader_.Uniform( "view_matrix", viewport_mat );
		floor_light_pass_shader_.Uniform( "world_matrix", world_mat );
		floor_light_pass_shader_.Uniform( "light_pos", light.pos );
		floor_light_pass_shader_.Uniform( "light_power", light.power / 128.0f );
		floor_light_pass_shader_.Uniform( "max_light_level", light.max_light_level / 128.0f  );
		floor_light_pass_shader_.Uniform( "min_radius", light.inner_radius );
		floor_light_pass_shader_.Uniform( "max_radius", light.outer_radius );

		glDrawArrays( GL_TRIANGLES, 0, 6 );
	}

	{ // Ambient light, drawn manually in map editor.
		r_Texture ambient_lightmap_texture(
			r_Texture::PixelFormat::R8,
			MapData::c_map_size, MapData::c_map_size,
			map_data->ambient_lightmap );
		ambient_lightmap_texture.SetFiltration( r_Texture::Filtration::Nearest, r_Texture::Filtration::Nearest );
		ambient_lightmap_texture.Bind(0);

		floor_ambient_light_pass_shader_.Bind();
		floor_ambient_light_pass_shader_.Uniform( "tex", 0 );

		glBlendEquation( GL_MAX );
		glDrawArrays( GL_TRIANGLES, 0, 6 );
		glBlendEquation( GL_FUNC_ADD );
	}

	r_Framebuffer::BindScreenFramebuffer();
}

const r_Texture& MapLight::GetFloorLightmap() const
{
	return final_floor_lightmap_.GetTextures().front();
}
const r_Texture& MapLight::GetWallsLightmap() const
{
	return final_walls_lightmap_.GetTextures().front();
}

} // namespace PanzerChasm
