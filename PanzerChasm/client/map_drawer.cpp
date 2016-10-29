#include <cstring>

#include <ogl_state_manager.hpp>

#include "../assert.hpp"
#include "../log.hpp"

#include "map_drawer.hpp"

namespace PanzerChasm
{

namespace
{

constexpr unsigned int g_wall_texture_height= 128u;
constexpr unsigned int g_max_wall_texture_width= 128u;

constexpr float g_walls_coords_scale= 256.0f;

const GLenum g_gl_state_blend_func[2]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
const r_OGLState g_walls_gl_state(
	false, false, true, false,
	g_gl_state_blend_func );

const r_OGLState g_floors_gl_state= g_walls_gl_state;

} // namespace

struct FloorVertex
{
	unsigned char xy[2];
	unsigned char texture_id;
	unsigned char reserved;
};

SIZE_ASSERT( FloorVertex, 4u );

struct WallVertex
{
	float tex_coord_x;
	unsigned short xyz[3]; // 8.8 fixed
	unsigned char texture_id;
	char normal[2];
	unsigned char reserved[3];
};

SIZE_ASSERT( WallVertex, 16u );

MapDrawer::MapDrawer(
	GameResourcesConstPtr game_resources,
	const RenderingContext& rendering_context )
	: game_resources_(std::move(game_resources))
	, rendering_context_(rendering_context)
{
	PC_ASSERT( game_resources_ != nullptr );

	// Textures
	glGenTextures( 1, &floor_textures_array_id_ );
	glGenTextures( 1, &wall_textures_array_id_ );
	glGenTextures( 1, &models_textures_array_id_ );

	// Shaders
	char lightmap_scale[32];
	std::snprintf( lightmap_scale, sizeof(lightmap_scale), "LIGHTMAP_SCALE %f", 1.0f / float(MapData::c_map_size) );

	const std::vector<std::string> defines{ lightmap_scale };

	floors_shader_.ShaderSource(
		rLoadShader( "floors_f.glsl", rendering_context.glsl_version ),
		rLoadShader( "floors_v.glsl", rendering_context. glsl_version, defines ) );
	floors_shader_.SetAttribLocation( "pos", 0u );
	floors_shader_.SetAttribLocation( "tex_id", 1u );
	floors_shader_.Create();

	walls_shader_.ShaderSource(
		rLoadShader( "walls_f.glsl", rendering_context.glsl_version ),
		rLoadShader( "walls_v.glsl", rendering_context.glsl_version, defines ) );
	walls_shader_.SetAttribLocation( "pos", 0u );
	walls_shader_.SetAttribLocation( "tex_coord_x", 1u );
	walls_shader_.SetAttribLocation( "tex_id", 2u );
	walls_shader_.SetAttribLocation( "normal", 3u );
	walls_shader_.Create();

	models_shader_.ShaderSource(
		rLoadShader( "models_f.glsl", rendering_context.glsl_version ),
		rLoadShader( "models_v.glsl", rendering_context.glsl_version ) );
	models_shader_.SetAttribLocation( "pos", 0u );
	models_shader_.SetAttribLocation( "tex_coord", 1u );
	models_shader_.SetAttribLocation( "tex_id", 2u );
	models_shader_.Create();
}

MapDrawer::~MapDrawer()
{
	glDeleteTextures( 1, &floor_textures_array_id_ );
	glDeleteTextures( 1, &wall_textures_array_id_ );
	glDeleteTextures( 1, &models_textures_array_id_ );
}

void MapDrawer::SetMap( const MapDataConstPtr& map_data )
{
	PC_ASSERT( map_data != nullptr );

	current_map_data_= map_data;

	LoadFloorsTextures( *map_data );
	LoadWallsTextures( *map_data );
	LoadFloors( *map_data );
	LoadWalls( *map_data );

	LoadModels( *map_data );

	lightmap_=
		r_Texture(
			r_Texture::PixelFormat::R8,
			MapData::c_lightmap_size, MapData::c_lightmap_size,
			map_data->lightmap );
	lightmap_.SetFiltration( r_Texture::Filtration::Nearest, r_Texture::Filtration::Nearest );
}

void MapDrawer::Draw(
	const MapState& map_state,
	const m_Mat4& view_matrix )
{
	if( current_map_data_ == nullptr )
		return;

	UpdateDynamicWalls( map_state.GetDynamicWalls() );

	// Draw walls
	r_OGLStateManager::UpdateState( g_walls_gl_state );

	walls_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, wall_textures_array_id_ );
	lightmap_.Bind(1);

	walls_shader_.Uniform( "tex", int(0) );
	walls_shader_.Uniform( "lightmap", int(1) );

	{
		m_Mat4 scale_mat;
		scale_mat.Scale( 1.0f / 256.0f );

		walls_shader_.Uniform( "view_matrix", scale_mat * view_matrix );
	}

	walls_geometry_.Bind();
	walls_geometry_.Draw();
	dynamic_walls_geometry_.Bind();
	dynamic_walls_geometry_.Draw();

	// Draw floors and ceilings
	r_OGLStateManager::UpdateState( g_floors_gl_state );

	floors_geometry_.Bind();
	floors_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, floor_textures_array_id_ );
	lightmap_.Bind(1);

	floors_shader_.Uniform( "tex", int(0) );
	floors_shader_.Uniform( "lightmap", int(1) );

	floors_shader_.Uniform( "view_matrix", view_matrix );

	for( unsigned int z= 0u; z < 2u; z++ )
	{
		floors_shader_.Uniform( "pos_z", float( 1u - z ) * 2.0f );

		glDrawArrays(
			GL_TRIANGLES,
			floors_geometry_info[z].first_vertex_number,
			floors_geometry_info[z].vertex_count );
	}
}

void MapDrawer::LoadFloorsTextures( const MapData& map_data )
{
	const Palette& palette= game_resources_->palette;

	const unsigned int texture_texels= MapData::c_floor_texture_size * MapData::c_floor_texture_size;

	glBindTexture( GL_TEXTURE_2D_ARRAY, floor_textures_array_id_ );
	glTexImage3D(
		GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
		MapData::c_floor_texture_size, MapData::c_floor_texture_size, MapData::c_floors_textures_count,
		0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );

	for( unsigned int t= 0u; t < MapData::c_floors_textures_count; t++ )
	{
		const unsigned char* const in_data= map_data.floor_textures_data[t];

		unsigned char texture_data[ 4u * texture_texels ];

		for( unsigned int i= 0u; i < texture_texels; i++ )
		{
			const unsigned char color_index= in_data[i];
			for( unsigned int j= 0u; j < 3u; j++ )
				texture_data[ (i << 2u) + j ]= palette[ color_index * 3u + j ];
			texture_data[ (i << 2u) + 3u ]= 255u;
		}

		glTexSubImage3D(
			GL_TEXTURE_2D_ARRAY, 0,
			0, 0, t,
			MapData::c_floor_texture_size, MapData::c_floor_texture_size, 1,
			GL_RGBA, GL_UNSIGNED_BYTE,
			texture_data );

	} // for textures

	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
	glGenerateMipmap( GL_TEXTURE_2D_ARRAY );
}

void MapDrawer::LoadWallsTextures( const MapData& map_data )
{
	const Palette& palette= game_resources_->palette;

	Vfs::FileContent texture_file;

	glBindTexture( GL_TEXTURE_2D_ARRAY, wall_textures_array_id_ );
	glTexImage3D(
		GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
		g_max_wall_texture_width, g_wall_texture_height, MapData::c_max_walls_textures,
		0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );

	for( unsigned int t= 0u; t < MapData::c_max_walls_textures; t++ )
	{
		if( map_data.walls_textures[t][0] == '\0' )
			continue;

		game_resources_->vfs->ReadFile( map_data.walls_textures[t], texture_file );
		if( texture_file.empty() )
			continue;

		unsigned char texture_data[ g_max_wall_texture_width * g_wall_texture_height * 4u ];

		unsigned short src_width, src_height;
		std::memcpy( &src_width , texture_file.data() + 0x2u, sizeof(unsigned short) );
		std::memcpy( &src_height, texture_file.data() + 0x4u, sizeof(unsigned short) );

		if( g_max_wall_texture_width / src_width * src_width != g_max_wall_texture_width ||
			src_height != g_wall_texture_height )
		{
			Log::Warning( "Invalid wall texture size: ", src_width, "x", src_height );
			continue;
		}

		const unsigned char* const src= texture_file.data() + 0x320u;

		for( unsigned int y= 0u; y < g_wall_texture_height; y++ )
		{
			const unsigned int y_flipped= g_wall_texture_height - 1u - y;

			for( unsigned int x= 0u; x < src_width; x++ )
			{
				const unsigned int color_index= src[ x + y_flipped * src_width ];
				const unsigned int i= ( x + y * g_max_wall_texture_width ) << 2;

				for( unsigned int j= 0u; j < 3u; j++ )
					texture_data[ i + j ]= palette[ color_index * 3u + j ];

				texture_data[ i + 3u ]= color_index == 255u ? 0u : 255u;
			}

			const unsigned int repeats= g_max_wall_texture_width / src_width;
			unsigned char* const line= texture_data + g_max_wall_texture_width * 4u * y;
			for( unsigned int r= 1u; r < repeats; r++ )
				std::memcpy(
					line + r * src_width * 4u,
					line,
					src_width * 4u );
		}

		glTexSubImage3D(
			GL_TEXTURE_2D_ARRAY, 0,
			0, 0, t,
			g_max_wall_texture_width, g_wall_texture_height, 1,
			GL_RGBA, GL_UNSIGNED_BYTE,
			texture_data );

	} // for wall textures

	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
	glGenerateMipmap( GL_TEXTURE_2D_ARRAY );
}

void MapDrawer::LoadFloors( const MapData& map_data )
{
	std::vector<FloorVertex> floors_vertices;
	for( unsigned int floor_or_ceiling= 0u; floor_or_ceiling < 2u; floor_or_ceiling++ )
	{
		floors_geometry_info[ floor_or_ceiling ].first_vertex_number= floors_vertices.size();

		const unsigned char* const in_data= floor_or_ceiling == 0u ? map_data.floor_textures : map_data.ceiling_textures;

		for( unsigned int x= 0u; x < MapData::c_map_size; x++ )
		for( unsigned int y= 0u; y < MapData::c_map_size; y++ )
		{
			const unsigned char texture_number= in_data[ x * MapData::c_map_size + y ] & 63u;
			if( texture_number == 63u )
				continue;

			floors_vertices.resize( floors_vertices.size() + 6u );
			FloorVertex* v= floors_vertices.data() + floors_vertices.size() - 6u;

			v[0].xy[0]= x   ;  v[0].xy[1]= y;
			v[1].xy[0]= x+1u;  v[1].xy[1]= y;
			v[2].xy[0]= x+1u;  v[2].xy[1]= y+1u;
			v[3].xy[0]= x   ;  v[3].xy[1]= y+1u;

			v[0].texture_id= v[1].texture_id= v[2].texture_id= v[3].texture_id= texture_number;
			v[4]= v[0];
			v[5]= v[2];
		} // for xy

		floors_geometry_info[ floor_or_ceiling ].vertex_count=
			floors_vertices.size() - floors_geometry_info[ floor_or_ceiling ].first_vertex_number;
	} // for floor and ceiling

	floors_geometry_.VertexData(
		floors_vertices.data(),
		floors_vertices.size() * sizeof(FloorVertex),
		sizeof(FloorVertex) );

	FloorVertex v;

	floors_geometry_.VertexAttribPointer(
		0, 2, GL_UNSIGNED_BYTE, false,
		((char*)v.xy) - ((char*)&v) );

	floors_geometry_.VertexAttribPointerInt(
		1, 1, GL_UNSIGNED_BYTE,
		((char*)&v.texture_id) - ((char*)&v) );
}

void MapDrawer::LoadWalls( const MapData& map_data )
{
	std::vector<WallVertex> walls_vertices( map_data.static_walls.size() * 4u );
	std::vector<unsigned short> walls_indeces( map_data.static_walls.size() * 6u );

	for( unsigned int w= 0u; w < map_data.static_walls.size(); w++ )
	{
		const MapData::Wall& wall= map_data.static_walls[w];
		// TODO - discard walls without textures.

		const unsigned int first_vertex_index= w * 4u;
		WallVertex* const v= walls_vertices.data() + first_vertex_index;

		v[0].xyz[0]= v[2].xyz[0]= static_cast<unsigned short>( wall.vert_pos[0].x * g_walls_coords_scale );
		v[0].xyz[1]= v[2].xyz[1]= static_cast<unsigned short>( wall.vert_pos[0].y * g_walls_coords_scale );
		v[1].xyz[0]= v[3].xyz[0]= static_cast<unsigned short>( wall.vert_pos[1].x * g_walls_coords_scale );
		v[1].xyz[1]= v[3].xyz[1]= static_cast<unsigned short>( wall.vert_pos[1].y * g_walls_coords_scale );

		v[0].xyz[2]= v[1].xyz[2]= 0u;
		v[2].xyz[2]= v[3].xyz[2]= 2u << 8u;

		v[0].texture_id= v[1].texture_id= v[2].texture_id= v[3].texture_id= wall.texture_id;

		v[2].tex_coord_x= v[0].tex_coord_x= wall.vert_tex_coord[0];
		v[1].tex_coord_x= v[3].tex_coord_x= wall.vert_tex_coord[1];

		m_Vec2 wall_vec=
			m_Vec2(float(v[0].xyz[0]), float(v[0].xyz[1])) -
			m_Vec2(float(v[1].xyz[0]), float(v[1].xyz[1]));
		wall_vec.Normalize();

		const int normal[2]= {
			int( +126.5f * wall_vec.y ),
			int( -126.5f * wall_vec.x ) };
		for( unsigned int j= 0u; j < 4u; j++ )
		{
			v[j].normal[0]= normal[0];
			v[j].normal[1]= normal[1];
		}

		unsigned short* const ind= walls_indeces.data() + w * 6u;
		ind[0]= first_vertex_index + 0u;
		ind[1]= first_vertex_index + 1u;
		ind[2]= first_vertex_index + 3u;
		ind[3]= first_vertex_index + 0u;
		ind[4]= first_vertex_index + 3u;
		ind[5]= first_vertex_index + 2u;
	} // for walls

	const auto setup_attribs=
	[]( r_PolygonBuffer& polygon_buffer )
	{
		WallVertex v;

		polygon_buffer.VertexAttribPointer(
			0, 3, GL_UNSIGNED_SHORT, false,
			((char*)v.xyz) - ((char*)&v) );

		polygon_buffer.VertexAttribPointer(
			1, 1, GL_FLOAT, false,
			((char*)&v.tex_coord_x) - ((char*)&v) );

		polygon_buffer.VertexAttribPointerInt(
			2, 1, GL_UNSIGNED_BYTE,
			((char*)&v.texture_id) - ((char*)&v) );

		polygon_buffer.VertexAttribPointer(
			3, 2, GL_BYTE, true,
			((char*)v.normal) - ((char*)&v) );
	};

	walls_geometry_.VertexData(
		walls_vertices.data(),
		walls_vertices.size() * sizeof(WallVertex),
		sizeof(WallVertex) );

	walls_geometry_.IndexData(
		walls_indeces.data(),
		walls_indeces.size() * sizeof(unsigned short),
		GL_UNSIGNED_SHORT,
		GL_TRIANGLES );

	setup_attribs( walls_geometry_ );

	// Reserve place for dynamic walls geometry
	dynamc_walls_vertices_.resize( map_data.dynamic_walls.size() * 4u );

	// Try reuse indeces from regular walls
	if( walls_indeces.size() < map_data.dynamic_walls.size() * 6u )
	{
		walls_indeces.resize( map_data.dynamic_walls.size() * 6u );

		for( unsigned int w= 0u; w < map_data.dynamic_walls.size(); w++ )
		{
			unsigned short* const ind= walls_indeces.data() + w * 6u;
			const unsigned int first_vertex_index= w * 4u;
			ind[0]= first_vertex_index + 0u;
			ind[1]= first_vertex_index + 1u;
			ind[2]= first_vertex_index + 3u;
			ind[3]= first_vertex_index + 0u;
			ind[4]= first_vertex_index + 3u;
			ind[5]= first_vertex_index + 2u;
		}
	}

	dynamic_walls_geometry_.VertexData(
		nullptr,
		sizeof(WallVertex) * map_data.dynamic_walls.size() * 4u,
		sizeof(WallVertex) );

	dynamic_walls_geometry_.IndexData(
		walls_indeces.data(),
		map_data.dynamic_walls.size() * 6u * sizeof(unsigned short),
		GL_UNSIGNED_SHORT,
		GL_TRIANGLES );

	setup_attribs( dynamic_walls_geometry_ );
}

void MapDrawer::LoadModels( const MapData& map_data )
{
	const Palette& palette= game_resources_->palette;

	const unsigned int model_count= map_data.models.size();
	models_geometry_.resize( model_count );

	// TODO - use atlas instead textures array
	const unsigned int c_max_texture_size[2]= { 64u, 1024u };
	const unsigned int c_texels_in_layer= c_max_texture_size[0u] * c_max_texture_size[1u];
	std::vector<unsigned char> textures_data_rgba( 4u * c_texels_in_layer * model_count, 0u );

	std::vector<unsigned  short> indeces;
	std::vector<Model::Vertex> vertices;

	for( unsigned int m= 0u; m < model_count; m++ )
	{
		const Model& model= map_data.models[m];
		ModelGeometry& model_geometry= models_geometry_[m];

		unsigned int model_texture_height= model.texture_size[1];
		if( model.texture_size[1u] > c_max_texture_size[1u] )
		{
			Log::Warning(
				"Model \"" , map_data.models_description[m].file_name,
				"\" texture height is too big: ", model.texture_size[1u] );
			model_texture_height= c_max_texture_size[1u];
		}

		// Copy texture into atlas.
		unsigned char* const texture_dst= textures_data_rgba.data() + 4u * c_texels_in_layer * m;
		for( unsigned int y= 0u; y < model_texture_height; y++ )
		for( unsigned int x= 0u; x < model.texture_size[0u]; x++ )
		{
			const unsigned int i= ( x + y * c_max_texture_size[0u] ) << 2u;
			const unsigned char color_index= model.texture_data[ x + y * model.texture_size[0u] ];
			for( unsigned int j= 0u; j < 3u; j++ )
				texture_dst[ i + j ]= palette[ color_index * 3u + j ];
		}

		// Fill free texture space
		for(
			unsigned int y= model_texture_height;
			y < ( ( c_max_texture_size[1u] + model_texture_height ) >> 1u );
			y++ )
		{
			std::memcpy(
				texture_dst + 4u * c_max_texture_size[0u] * y,
				texture_dst + 4u * c_max_texture_size[0u] * ( model.texture_size[1u] - 1u ),
				4u * c_max_texture_size[0u] );
		}
		for(
			unsigned int y= ( c_max_texture_size[1u] + model_texture_height )>> 1u;
			y < c_max_texture_size[1u];
			y++ )
		{
			std::memcpy(
				texture_dst + 4u * c_max_texture_size[0u] * y,
				texture_dst,
				4u * c_max_texture_size[0u] );
		}

		// Copy vertices, transform textures coordinates, set texture layer.
		const unsigned int first_vertex_index= vertices.size();
		vertices.resize( vertices.size() + model.vertices.size() );
		Model::Vertex* const vertex= vertices.data() + first_vertex_index;

		const float tex_coord_scaler[2u]=
		{
			float(model.texture_size[0u]) / float(c_max_texture_size[0u]),
			float(model.texture_size[1u]) / float(c_max_texture_size[1u]),
		};
		for( unsigned int v= 0u; v < model.vertices.size(); v++ )
		{
			vertex[v]= model.vertices[v];
			vertex[v].texture_id= m;
			for( unsigned int j= 0u; j < 2u; j++ )
				vertex[v].tex_coord[j]*= tex_coord_scaler[j];
		}

		// Copy indeces.
		const unsigned int first_index= indeces.size();
		indeces.resize( indeces.size() + model.regular_triangles_indeces.size() + model.transparent_triangles_indeces.size() );
		unsigned short* const ind= indeces.data() + first_index;

		std::memcpy(
			ind,
			model.regular_triangles_indeces.data(),
			model.regular_triangles_indeces.size() * sizeof(unsigned short) );

		std::memcpy(
			ind + model.regular_triangles_indeces.size(),
			model.transparent_triangles_indeces.data(),
			model.transparent_triangles_indeces.size() * sizeof(unsigned short) );

		// Setup geometry info.
		model_geometry.frame_count= model.frame_count;
		model_geometry.vertex_count= model.vertices.size() / model.frame_count;
		model_geometry.first_vertex_index= first_vertex_index;

		model_geometry.first_index= first_index;
		model_geometry.index_count= model.regular_triangles_indeces.size();

		model_geometry.first_transparent_index= first_index + model.regular_triangles_indeces.size();
		model_geometry.transparent_index_count= model.transparent_triangles_indeces.size();

	} // for models

	// Prepare texture.
	glBindTexture( GL_TEXTURE_2D_ARRAY, models_textures_array_id_ );
	glTexImage3D(
		GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
		c_max_texture_size[0u], c_max_texture_size[1u], model_count,
		0, GL_RGBA, GL_UNSIGNED_BYTE, textures_data_rgba.data() );

	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
	glGenerateMipmap( GL_TEXTURE_2D_ARRAY );

	// Prepare vertex and index buffer.
	models_geometry_data_.VertexData(
		vertices.data(),
		vertices.size() * sizeof(Model::Vertex),
		sizeof(Model::Vertex) );

	models_geometry_data_.IndexData(
		indeces.data(),
		indeces.size() * sizeof(unsigned short),
		GL_UNSIGNED_SHORT,
		GL_TRIANGLES );

	Model::Vertex v;
	models_geometry_data_.VertexAttribPointer(
		0, 3, GL_FLOAT, false,
		((char*)v.pos) - ((char*)&v) );

	models_geometry_data_.VertexAttribPointer(
		1, 3, GL_FLOAT, false,
		((char*)v.tex_coord) - ((char*)&v) );

	models_geometry_data_.VertexAttribPointerInt(
		2, 1, GL_UNSIGNED_BYTE,
		((char*)&v.texture_id) - ((char*)&v) );
}

void MapDrawer::UpdateDynamicWalls( const MapState::DynamicWalls& dynamic_walls )
{
	PC_ASSERT( current_map_data_->dynamic_walls.size() == dynamic_walls.size() );

	for( unsigned int w= 0u; w < dynamic_walls.size(); w++ )
	{
		const MapData::Wall& map_wall= current_map_data_->dynamic_walls[w];
		const MapState::DynamicWall wall= dynamic_walls[w];
		// TODO - discard walls without textures.

		WallVertex* const v= dynamc_walls_vertices_.data() + w * 4u;

		v[0].xyz[0]= v[2].xyz[0]= wall.xy[0][0];
		v[0].xyz[1]= v[2].xyz[1]= wall.xy[0][1];
		v[1].xyz[0]= v[3].xyz[0]= wall.xy[1][0];
		v[1].xyz[1]= v[3].xyz[1]= wall.xy[1][1];

		v[0].xyz[2]= v[1].xyz[2]= wall.z;
		v[2].xyz[2]= v[3].xyz[2]= wall.z + (2u << 8u);

		v[0].texture_id= v[1].texture_id= v[2].texture_id= v[3].texture_id= map_wall.texture_id;

		v[2].tex_coord_x= v[0].tex_coord_x= map_wall.vert_tex_coord[0];
		v[1].tex_coord_x= v[3].tex_coord_x= map_wall.vert_tex_coord[1];

		m_Vec2 wall_vec=
			m_Vec2(float(v[0].xyz[0]), float(v[0].xyz[1])) -
			m_Vec2(float(v[1].xyz[0]), float(v[1].xyz[1]));
		wall_vec.Normalize();

		const int normal[2]= {
			int( +126.5f * wall_vec.y ),
			int( -126.5f * wall_vec.x ) };
		for( unsigned int j= 0u; j < 4u; j++ )
		{
			v[j].normal[0]= normal[0];
			v[j].normal[1]= normal[1];
		}
	}

	dynamic_walls_geometry_.VertexSubData(
		dynamc_walls_vertices_.data(),
		dynamc_walls_vertices_.size() * sizeof(WallVertex),
		0u );
}

} // PanzerChasm
