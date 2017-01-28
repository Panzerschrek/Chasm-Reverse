#include <cstring>

#include <ogl_state_manager.hpp>

#include "../game_resources.hpp"
#include "../map_loader.hpp"
#include "map_state.hpp"

#include "map_light.hpp"

namespace PanzerChasm
{

namespace
{

const float g_floor_light_scale= 0.8f / 128.0f;
const float g_walls_light_scale= 1.2f / 128.0f;

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


MapLight::MapLight(
	const GameResourcesConstPtr& game_resources,
	const RenderingContext& rendering_context )
	: game_resources_(game_resources)
{
	PC_ASSERT( game_resources_ != nullptr );

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

	walls_light_pass_shader_.ShaderSource(
		rLoadShader( "walls_light_f.glsl", rendering_context.glsl_version ),
		rLoadShader( "walls_light_v.glsl", rendering_context.glsl_version ) );
	walls_light_pass_shader_.SetAttribLocation( "pos", 0 );
	walls_light_pass_shader_.SetAttribLocation( "lightmap_coord", 1 );
	walls_light_pass_shader_.SetAttribLocation( "normal", 2 );
	walls_light_pass_shader_.Create();

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
	PrepareMapWalls( *map_data );

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

	base_walls_lightmap_.Bind();
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

		r_OGLStateManager::UpdateState( g_light_pass_state );

		// Add light to floor lightmap.
		base_floor_lightmap_.Bind();
		floor_light_pass_shader_.Bind();
		DrawLight( light );

		// Add light to walls lightmap.
		base_walls_lightmap_.Bind();
		walls_light_pass_shader_.Bind();

		walls_light_pass_shader_.Uniform( "shadowmap", int(0) );
		walls_light_pass_shader_.Uniform( "light_pos", light.pos );
		walls_light_pass_shader_.Uniform( "light_power", light.power * g_walls_light_scale );
		walls_light_pass_shader_.Uniform( "max_light_level", light.max_light_level * g_walls_light_scale );
		walls_light_pass_shader_.Uniform( "min_radius", light.inner_radius );
		walls_light_pass_shader_.Uniform( "max_radius", light.outer_radius );

		walls_vertex_buffer_.Draw();
	}

	r_Framebuffer::BindScreenFramebuffer();

	// Update ambient light texture.
	ambient_lightmap_texture_=
		r_Texture(
			r_Texture::PixelFormat::R8,
			MapData::c_map_size, MapData::c_map_size,
			map_data->ambient_lightmap );
	ambient_lightmap_texture_.SetFiltration( r_Texture::Filtration::Nearest, r_Texture::Filtration::Nearest );
}

void MapLight::Update( const MapState& map_state )
{
	PC_UNUSED( map_state );

	// Clear shadowmam.
	shadowmap_.Bind();
	r_OGLStateManager::UpdateState( g_lightmap_clear_state );
	glClear( GL_COLOR_BUFFER_BIT );

	// Draw to floor lightmap.
	final_floor_lightmap_.Bind();

	{ // Copy base floor lightmap.
		r_OGLStateManager::UpdateState( g_lightmap_clear_state );
		floor_ambient_light_pass_shader_.Bind();
		base_floor_lightmap_.GetTextures().front().Bind(0);
		floor_ambient_light_pass_shader_.Uniform( "tex", 0 );
		glDrawArrays( GL_TRIANGLES, 0, 6 );
	}

	if( false )
	{ // Mix with ambient light texture.
		r_OGLStateManager::UpdateState( g_light_pass_state );
		floor_ambient_light_pass_shader_.Bind();
		ambient_lightmap_texture_.Bind(0);
		floor_ambient_light_pass_shader_.Uniform( "tex", 0 );

		glBlendEquation( GL_MAX );
		glDrawArrays( GL_TRIANGLES, 0, 6 );
		glBlendEquation( GL_FUNC_ADD );
	}

	{ // Dynamic lights.
		r_OGLStateManager::UpdateState( g_light_pass_state );
		floor_light_pass_shader_.Bind();

		for( const MapState::RocketsContainer::value_type& rocket_value : map_state.GetRockets() )
		{
			const MapState::Rocket& rocket= rocket_value.second;
			if( rocket.rocket_id >= game_resources_->rockets_description.size() )
				continue;
			if( !game_resources_->rockets_description[ rocket.rocket_id ].Light )
				continue;

			MapData::Light light;
			light.inner_radius= 0.5f;
			light.outer_radius= 1.0f;
			light.power= 64.0f;
			light.max_light_level= 128.0f;
			light.pos= rocket.pos.xy();
			DrawLight( light );
		}
	}

	PrepareMapWalls( *map_data_ );

	r_Framebuffer::BindScreenFramebuffer();
}

void MapLight::GetStaticWallLightmapCoord(
	const unsigned int static_wall_index,
	unsigned char* out_coord_xy ) const
{
	PC_ASSERT( static_wall_index < map_data_->static_walls.size() );
	std::memcpy(
		out_coord_xy,
		walls_vertices_[ static_walls_first_vertex_ + 2u * static_wall_index ].lightmap_coord_xy,
		2u );
}

void MapLight::GetDynamicWallLightmapCoord(
	const unsigned int dynamic_wall_index,
	unsigned char* out_coord_xy ) const
{
	PC_ASSERT( dynamic_wall_index < map_data_->dynamic_walls.size() );
	std::memcpy(
		out_coord_xy,
		walls_vertices_[ dynamic_walls_first_vertex_ + 2u * dynamic_wall_index ].lightmap_coord_xy,
		2u );
}

const r_Texture& MapLight::GetFloorLightmap() const
{
	return final_floor_lightmap_.GetTextures().front();
}

const r_Texture& MapLight::GetWallsLightmap() const
{
	//return final_walls_lightmap_.GetTextures().front();
	return base_walls_lightmap_.GetTextures().front();
}

void MapLight::PrepareMapWalls( const MapData& map_data )
{
	walls_vertices_.resize( 2u * ( map_data.static_walls.size() + map_data.dynamic_walls.size() ) );
	static_walls_first_vertex_= 0u;
	dynamic_walls_first_vertex_= map_data.static_walls.size() * 2u;

	// Place walls in lightmap atlas.
	unsigned int x= 0u, y= 0u;

	const auto build_wall=
	[&]( const MapData::Wall& in_wall, WallVertex* const v )
	{
		v[0].pos[0]= short( in_wall.vert_pos[0].x * 256.0f );
		v[0].pos[1]= short( in_wall.vert_pos[0].y * 256.0f );
		v[1].pos[0]= short( in_wall.vert_pos[1].x * 256.0f );
		v[1].pos[1]= short( in_wall.vert_pos[1].y * 256.0f );

		v[0].lightmap_coord_xy[0]= x;
		v[0].lightmap_coord_xy[1]= y;
		v[1].lightmap_coord_xy[0]= x + 1u;
		v[1].lightmap_coord_xy[1]= y;

		m_Vec2 normal( in_wall.vert_pos[1].y - in_wall.vert_pos[0].y, in_wall.vert_pos[0].x - in_wall.vert_pos[1].x );
		normal.Normalize();

		v[0].normal[0]= v[1].normal[0]= static_cast<unsigned char>( normal.x * 126.5f );
		v[0].normal[1]= v[1].normal[1]= static_cast<unsigned char>( normal.y * 126.5f );

		x++;
		if( x >= MapData::c_map_size )
		{
			x= 0u;
			y++;
		}
	};

	for( unsigned int w= 0u; w < map_data.static_walls.size(); w++ )
		build_wall(
			map_data.static_walls[w],
			&walls_vertices_[ static_walls_first_vertex_ + w * 2u ] );

	for( unsigned int w= 0u; w < map_data.dynamic_walls.size(); w++ )
		build_wall(
			map_data.dynamic_walls[w],
			&walls_vertices_[ dynamic_walls_first_vertex_ + w * 2u ] );

	// Send data to GPU.
	walls_vertex_buffer_.VertexData(
		walls_vertices_.data(),
		sizeof(WallVertex) * walls_vertices_.size(),
		sizeof(WallVertex) );

	walls_vertex_buffer_.SetPrimitiveType( GL_LINES );

	WallVertex v;
	walls_vertex_buffer_.VertexAttribPointer( 0, 2, GL_SHORT, false, ((char*)v.pos) - (char*)&v );
	walls_vertex_buffer_.VertexAttribPointer( 1, 2, GL_UNSIGNED_BYTE, false, ((char*)v.lightmap_coord_xy) - (char*)&v );
	walls_vertex_buffer_.VertexAttribPointer( 2, 2, GL_BYTE, true, ((char*)v.normal) - (char*)&v );
}

void MapLight::DrawLight( const MapData::Light& light )
{
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
	floor_light_pass_shader_.Uniform( "light_power", light.power * g_floor_light_scale );
	floor_light_pass_shader_.Uniform( "max_light_level", light.max_light_level * g_floor_light_scale );
	floor_light_pass_shader_.Uniform( "min_radius", light.inner_radius );
	floor_light_pass_shader_.Uniform( "max_radius", light.outer_radius );

	glDrawArrays( GL_TRIANGLES, 0, 6 );
}

} // namespace PanzerChasm
