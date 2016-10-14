#include <cstring>
#include <sstream>
#include <iostream>

#include <shaders_loading.hpp>

#include "math_utils.hpp"
#include "model.hpp"

#include "map_viewer.hpp"

namespace ChasmReverse
{

static const unsigned int g_floor_texture_size= 64u;
static const unsigned int g_floor_texture_texels= g_floor_texture_size * g_floor_texture_size;
static const unsigned int g_floor_textures_in_level= 64u;
static const unsigned int g_floors_file_header_size= 64u;
static const unsigned int g_bytes_per_floor_in_file= 86u * g_floor_texture_size;

static const unsigned int g_map_size= 64u; // in cells
static const unsigned int g_map_cells= g_map_size * g_map_size;

static const unsigned int g_lightmap_texels_per_cell= 4u;
static const unsigned int g_lightmap_size= g_map_size * g_lightmap_texels_per_cell;
static const unsigned int g_lightmap_texels= g_lightmap_size * g_lightmap_size;

static const unsigned int g_wall_texture_height= 128u;
static const unsigned int g_max_wall_texture_width= 128u;
static const unsigned int g_max_wall_textures= 128u;

#define SIZE_ASSERT(x, size) static_assert( sizeof(x) == size, "Invalid size" )

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

#pragma pack(push, 1)
struct MapWall
{
	unsigned char texture_id;
	unsigned char wall_size;
	unsigned char unknown;
	unsigned short vert_coord[2][2];
};
#pragma pack(pop)

SIZE_ASSERT( MapWall, 11u );

typedef std::array<char[16u], 256u> WallsTexturesNames;

struct ModelDescription
{
	char model_file_name[16u];
	char animation_file_name[16u];
};

typedef std::array<ModelDescription, 128u> ModeslDescription;

void LoadWallsTexturesNames( const Vfs::FileContent& resources_file, WallsTexturesNames& out_names )
{
	// Very complex stuff here.
	// TODO - use parser.

	for( char* const file_name : out_names )
		file_name[0]= '\0';

	const char* const gfx_section= std::strstr( reinterpret_cast<const char*>( resources_file.data() ), "#GFX" );
	const char* const gfx_section_end= std::strstr( gfx_section, "#end" );
	const char* stream= gfx_section + std::strlen( "#GFX" );

	while( stream < gfx_section_end )
	{
		while( *stream != '\n' ) stream++;
		while( *stream == '\r' ) stream++;
		stream++;

		if( stream >= gfx_section_end ) break;

		const unsigned int texture_slot= std::atoi( stream );

		while( *stream != ':' ) stream++;
		stream++;

		while( std::isspace( *stream ) && *stream != '\n' ) stream++;
		if( *stream == '\n' )
			continue;

		char* dst= out_names[ texture_slot ];
		while( !std::isspace( *stream ) )
		{
			*dst= *stream;
			dst++; stream++;
		}
		*dst= '\0';
	}
}

unsigned int LoadModelsNames( const Vfs::FileContent& resources_file, ModeslDescription& out_names )
{
	const char* start= std::strstr( reinterpret_cast<const char*>( resources_file.data() ), "#newobjects" );

	while( *start != '\n' ) start++;
	start++;

	const char* const end= std::strstr( start, "#end" );

	std::istringstream stream( std::string( start, end ) );

	unsigned int count= 0u;
	while( !stream.eof() )
	{
		char line[ 512u ];
		stream.getline( line, sizeof(line), '\n' );

		if( stream.eof() )
			break;
		std::istringstream line_stream{ std::string( line ) };

		double num;
		line_stream >> num; // GoRad
		line_stream >> num; // Shad
		line_stream >> num; // BObj
		line_stream >> num; // BMPz
		line_stream >> num; // AC
		line_stream >> num; // Blw
		line_stream >> num; // BLmt
		line_stream >> num; // SFX
		line_stream >> num; // BSfx

		line_stream >> out_names[ count ].model_file_name; // FileName

		out_names[ count ].animation_file_name[0u]= '\0';
		line_stream >> out_names[ count ].animation_file_name; // Animation

		count++;
	}

	return count;
}

MapViewer::MapViewer( const std::shared_ptr<Vfs>& vfs, unsigned int map_number )
{
	char map_file_name[16];

	// Load palette and convert to 8 bit from 6 bit.
	Vfs::FileContent palette= vfs->ReadFile( "CHASM2.PAL" );
	for( unsigned char& palette_element : palette )
		palette_element<<= 2u;

	std::snprintf( map_file_name, sizeof(map_file_name), "FLOORS.%02u", map_number );
	LoadFloorsTextures( vfs->ReadFile( map_file_name ), palette.data() );

	std::snprintf( map_file_name, sizeof(map_file_name), "MAP.%02u", map_number );
	const Vfs::FileContent map_file= vfs->ReadFile( map_file_name );

	// Load lightmap
	{
		unsigned char lightmap_transformed[ g_lightmap_texels ];

		const unsigned char* const in_data= map_file.data() + 0x01u;

		// TODO - tune formula
		for( unsigned int y= 0u; y < g_lightmap_size; y++ )
		for( unsigned int x= 0u; x < g_lightmap_size - 1u; x++ )
		{
			lightmap_transformed[ x + 1u + y * g_lightmap_size ]=
				255u - ( ( in_data[ x + y * g_lightmap_size ] - 3u ) * 6u );
		}

		lightmap_= r_Texture( r_Texture::PixelFormat::R8, g_lightmap_size, g_lightmap_size, lightmap_transformed );
		lightmap_.SetFiltration( r_Texture::Filtration::Nearest, r_Texture::Filtration::Nearest );
	}

	// Load floors geometry
	std::vector<FloorVertex> floors_vertices;
	for( unsigned int floor_or_ceiling= 0u; floor_or_ceiling < 2u; floor_or_ceiling++ )
	{
		floors_geometry_info[ floor_or_ceiling ].first_vertex_number= floors_vertices.size();

		const unsigned char* const in_data= map_file.data() + 0x23001u + g_map_cells * floor_or_ceiling;

		for( unsigned int x= 0u; x < g_map_size; x++ )
		for( unsigned int y= 0u; y < g_map_size; y++ )
		{
			unsigned char texture_number= in_data[ x * g_map_size + y ] & 63u;
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

	{
		FloorVertex v;

		floors_geometry_.VertexAttribPointer(
			0, 2, GL_UNSIGNED_BYTE, false,
			((char*)v.xy) - ((char*)&v) );

		floors_geometry_.VertexAttribPointerInt(
			1, 1, GL_UNSIGNED_BYTE,
			((char*)&v.texture_id) - ((char*)&v) );
	}

	std::snprintf( map_file_name, sizeof(map_file_name), "RESOURCE.%02u", map_number );
	const Vfs::FileContent resource_file= vfs->ReadFile( map_file_name );

	// Walls textures
	bool wall_texture_exist[ g_max_wall_textures ];
	LoadWallsTextures( *vfs, resource_file, palette.data(), wall_texture_exist );

	// Load walls geometry
	std::vector<WallVertex> walls_vertices;
	std::vector<unsigned short> walls_indeces;
	for( unsigned int y= 0u; y < g_map_size; y++ )
	for( unsigned int x= 1u; x < g_map_size; x++ )
	{
		const MapWall& map_wall=
			*reinterpret_cast<const MapWall*>( map_file.data() + 0x18001u + sizeof(MapWall) * ( x + y * g_map_size ) );

		if( map_wall.texture_id >= 128u )
		{
			const int model_id= int(map_wall.texture_id) - 163;
			if( model_id >= 0 )
			{
				level_models_.emplace_back();
				LevelModel& model= level_models_.back();
				model.pos.x= float(map_wall.vert_coord[0][0]) / 256.0f;
				model.pos.y= float(map_wall.vert_coord[0][1]) / 256.0f;
				model.pos.z= 0.0f;
				model.angle= float(map_wall.unknown & 7u) / 8.0f * Constants::two_pi + Constants::pi;
				model.id= model_id;
			}
			continue;
		}

		if(
			!wall_texture_exist[ map_wall.texture_id ] )
			continue;

		if( !( map_wall.wall_size == 64u || map_wall.wall_size == 128u ) )
			continue;

		const unsigned int first_vertex_index= walls_vertices.size();
		walls_vertices.resize( walls_vertices.size() + 4u );
		WallVertex* const v= walls_vertices.data() + first_vertex_index;

		v[0].xyz[0]= v[2].xyz[0]= map_wall.vert_coord[0][0];
		v[0].xyz[1]= v[2].xyz[1]= map_wall.vert_coord[0][1];
		v[1].xyz[0]= v[3].xyz[0]= map_wall.vert_coord[1][0];
		v[1].xyz[1]= v[3].xyz[1]= map_wall.vert_coord[1][1];

		v[0].xyz[2]= v[1].xyz[2]= 0u;
		v[2].xyz[2]= v[3].xyz[2]= 2u << 8u;

		v[0].texture_id= v[1].texture_id= v[2].texture_id= v[3].texture_id= map_wall.texture_id;

		v[2].tex_coord_x= v[0].tex_coord_x= float(map_wall.wall_size) / float(g_max_wall_texture_width);
		v[1].tex_coord_x= v[3].tex_coord_x= 0.0f;

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

		walls_indeces.resize( walls_indeces.size() + 6u );
		unsigned short* const ind= walls_indeces.data() + walls_indeces.size() - 6u;
		ind[0]= first_vertex_index + 0u;
		ind[1]= first_vertex_index + 1u;
		ind[2]= first_vertex_index + 3u;
		ind[3]= first_vertex_index + 0u;
		ind[4]= first_vertex_index + 3u;
		ind[5]= first_vertex_index + 2u;
	}

	walls_geometry_.VertexData(
		walls_vertices.data(),
		walls_vertices.size() * sizeof(WallVertex),
		sizeof(WallVertex) );

	walls_geometry_.IndexData(
		walls_indeces.data(),
		walls_indeces.size() * sizeof(unsigned short),
		GL_UNSIGNED_SHORT,
		GL_TRIANGLES );

	{
		WallVertex v;

		walls_geometry_.VertexAttribPointer(
			0, 3, GL_UNSIGNED_SHORT, false,
			((char*)v.xyz) - ((char*)&v) );

		walls_geometry_.VertexAttribPointer(
			1, 1, GL_FLOAT, false,
			((char*)&v.tex_coord_x) - ((char*)&v) );

		walls_geometry_.VertexAttribPointerInt(
			2, 1, GL_UNSIGNED_BYTE,
			((char*)&v.texture_id) - ((char*)&v) );

		walls_geometry_.VertexAttribPointer(
			3, 2, GL_BYTE, true,
			((char*)v.normal) - ((char*)&v) );
	}

	// Shaders
	{
		const r_GLSLVersion glsl_version( r_GLSLVersion::KnowmNumbers::v330, r_GLSLVersion::Profile::Core );

		char lightmap_scale[32];
		std::snprintf( lightmap_scale, sizeof(lightmap_scale), "LIGHTMAP_SCALE %f", 1.0f / float(g_map_size) );

		const std::vector<std::string> defines{ lightmap_scale };

		floors_shader_.ShaderSource(
			rLoadShader( "floors_f.glsl", glsl_version ),
			rLoadShader( "floors_v.glsl", glsl_version, defines ) );
		floors_shader_.SetAttribLocation( "pos", 0u );
		floors_shader_.SetAttribLocation( "tex_id", 1u );
		floors_shader_.Create();

		walls_shader_.ShaderSource(
			rLoadShader( "walls_f.glsl", glsl_version ),
			rLoadShader( "walls_v.glsl", glsl_version, defines ) );
		walls_shader_.SetAttribLocation( "pos", 0u );
		walls_shader_.SetAttribLocation( "tex_coord_x", 1u );
		walls_shader_.SetAttribLocation( "tex_id", 2u );
		walls_shader_.SetAttribLocation( "normal", 3u );
		walls_shader_.Create();

		models_shader_.ShaderSource(
			rLoadShader( "models_f.glsl", glsl_version ),
			rLoadShader( "models_v.glsl", glsl_version ) );
		models_shader_.SetAttribLocation( "pos", 0u );
		models_shader_.SetAttribLocation( "tex_coord", 1u );
		models_shader_.SetAttribLocation( "tex_id", 2u );
		models_shader_.Create();

		single_texture_models_shader_.ShaderSource(
			rLoadShader( "single_texture_models_f.glsl", glsl_version ),
			rLoadShader( "models_v.glsl", glsl_version ) );
		single_texture_models_shader_.SetAttribLocation( "pos", 0u );
		single_texture_models_shader_.SetAttribLocation( "tex_coord", 1u );
		single_texture_models_shader_.SetAttribLocation( "tex_id", 2u );
		single_texture_models_shader_.Create();
	}

	LoadModels( *vfs, resource_file, palette.data() );


	{
		const Vfs::FileContent car_model= vfs->ReadFile( "JOKER6.CAR" );

		Model loaded_model;
		LoadModel_car( car_model, loaded_model );
		{ // texture
			std::vector<unsigned char> texture_data_rgba( 4u * loaded_model.texture_data.size() );
			for( unsigned int i= 0u; i < loaded_model.texture_data.size(); i++ )
			{
				for( unsigned int j= 0u; j < 3u; j++ )
					texture_data_rgba[ i * 4u + j ]= palette[ loaded_model.texture_data[i] * 3u + j ];
			}

			test_model_texture_=
				r_Texture(
					r_Texture::PixelFormat::RGBA8,
					loaded_model.texture_size[0u], loaded_model.texture_size[1u],
					texture_data_rgba.data() );

			test_model_texture_.SetFiltration( r_Texture::Filtration::NearestMipmapLinear, r_Texture::Filtration::Nearest );
			test_model_texture_.BuildMips();
		}

		test_model_.first_index= 0u;
		test_model_.first_transparent_index= 0u;
		test_model_.first_vertex_index= 0u;
		test_model_.frame_count= loaded_model.frame_count;
		test_model_.index_count= loaded_model.regular_triangles_indeces.size();
		test_model_.transparent_index_count= 0u;
		test_model_.vertex_count= loaded_model.vertices.size() / loaded_model.frame_count;

		for( Model::Vertex& v : loaded_model.vertices )
			v.texture_id= 0u;

		test_model_vbo_.VertexData(
			loaded_model.vertices.data(),
			loaded_model.vertices.size() * sizeof(Model::Vertex),
			sizeof(Model::Vertex) );

		test_model_vbo_.IndexData(
			loaded_model.regular_triangles_indeces.data(),
			loaded_model.regular_triangles_indeces.size() * sizeof(unsigned short),
			GL_UNSIGNED_SHORT,
			GL_TRIANGLES );

		Model::Vertex v;
		test_model_vbo_.VertexAttribPointer(
			0, 3, GL_FLOAT, false,
			((char*)v.pos) - ((char*)&v) );

		test_model_vbo_.VertexAttribPointer(
			1, 3, GL_FLOAT, false,
			((char*)v.tex_coord) - ((char*)&v) );

		test_model_vbo_.VertexAttribPointerInt(
			2, 1, GL_UNSIGNED_BYTE,
			((char*)&v.texture_id) - ((char*)&v) );
	}
}

MapViewer::~MapViewer()
{
	glDeleteTextures( 1, &floor_textures_array_id_ );
	glDeleteTextures( 1, &wall_textures_array_id_ );
	glDeleteTextures( 1, &models_textures_array_id_ );
}

void MapViewer::Draw( const m_Mat4& view_matrix )
{
	// Draw walls
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

	// Draw floors and ceilings
	floors_geometry_.Bind();
	floors_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, floor_textures_array_id_ );
	lightmap_.Bind(1);

	floors_shader_.Uniform( "tex", int(0) );
	floors_shader_.Uniform( "lightmap", int(1) );

	floors_shader_.Uniform( "view_matrix", view_matrix );

	for( unsigned int z= 0; z < 2; z++ )
	{
		floors_shader_.Uniform( "pos_z", float( 1u - z ) * 2.0f );

		glDrawArrays(
			GL_TRIANGLES,
			floors_geometry_info[z].first_vertex_number,
			floors_geometry_info[z].vertex_count );
	}

	// Test model
	models_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, models_textures_array_id_ );

	models_shader_.Uniform( "tex", int(0) );

	models_geometry_data_.Bind();

	glEnable( GL_CULL_FACE );

	// Regular models geometry
	for( const LevelModel& level_model : level_models_ )
	{
		m_Mat4 rotate_mat, shift_mat;

		rotate_mat.RotateZ( level_model.angle );
		shift_mat.Translate( level_model.pos );

		models_shader_.Uniform( "view_matrix", rotate_mat * shift_mat * view_matrix );

		if( level_model.id >= models_geometry_.size() )
			continue;

		const ModelGeometry& model= models_geometry_[ level_model.id ];
		const unsigned int model_frame= ( frame_count_ / 3u ) % model.frame_count;

		glDrawElementsBaseVertex(
			GL_TRIANGLES,
			model.index_count,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>( model.first_index * sizeof(unsigned short) ),
			model.first_vertex_index + model_frame * model.vertex_count );
	}

	// Transpaent models geometry
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	for( const LevelModel& level_model : level_models_ )
	{
		m_Mat4 rotate_mat, shift_mat;

		rotate_mat.RotateZ( level_model.angle );
		shift_mat.Translate( level_model.pos );

		models_shader_.Uniform( "view_matrix", rotate_mat * shift_mat * view_matrix );

		if( level_model.id >= models_geometry_.size() )
			continue;

		const ModelGeometry& model= models_geometry_[ level_model.id ];
		const unsigned int model_frame= ( frame_count_ / 3u ) % model.frame_count;

		glDrawElementsBaseVertex(
			GL_TRIANGLES,
			model.transparent_index_count,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>( model.first_transparent_index * sizeof(unsigned short) ),
			model.first_vertex_index + model_frame * model.vertex_count );
	}
	glDisable( GL_BLEND );

	{ // test model

		single_texture_models_shader_.Bind();

		test_model_texture_.Bind(0);

		single_texture_models_shader_.Uniform( "view_matrix", view_matrix );
		single_texture_models_shader_.Uniform( "tex", int(0) );

		test_model_vbo_.Bind();

		const unsigned int model_frame= ( frame_count_ / 3u ) % test_model_.frame_count;

		glDrawElementsBaseVertex(
			GL_TRIANGLES,
			test_model_.index_count,
			GL_UNSIGNED_SHORT,
			reinterpret_cast<void*>( test_model_.first_index * sizeof(unsigned short) ),
			test_model_.first_vertex_index + model_frame * test_model_.vertex_count );
	}

	glDisable( GL_CULL_FACE );

	// End frame
	frame_count_++;
}

void MapViewer::LoadFloorsTextures(
	const Vfs::FileContent& floors_file,
	const unsigned char* const palette )
{
	glGenTextures( 1, &floor_textures_array_id_ );
	glBindTexture( GL_TEXTURE_2D_ARRAY, floor_textures_array_id_ );
	glTexImage3D(
		GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
		g_floor_texture_size, g_floor_texture_size, g_floor_textures_in_level,
		0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );

	for( unsigned int t= 0u; t < g_floor_textures_in_level; t++ )
	{
		const unsigned char* const in_data=
			floors_file.data() + g_floors_file_header_size + g_bytes_per_floor_in_file * t;

		unsigned char texture_data[ 4u * g_floor_texture_texels ];

		for( unsigned int i= 0u; i < g_floor_texture_texels; i++ )
		{
			const unsigned char color_index= in_data[i];
			for( unsigned int j= 0; j < 3u; j++ )
				texture_data[ (i << 2u) + j ]= palette[ color_index * 3u + j ];
			texture_data[ (i << 2u) + 3 ]= 255;
		}

		glTexSubImage3D(
			GL_TEXTURE_2D_ARRAY, 0,
			0, 0, t,
			g_floor_texture_size, g_floor_texture_size, 1,
			GL_RGBA, GL_UNSIGNED_BYTE,
			texture_data );

	} // for textures

	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
	glGenerateMipmap( GL_TEXTURE_2D_ARRAY );
}

void MapViewer::LoadWallsTextures(
	const Vfs& vfs,
	const Vfs::FileContent& resources_file,
	const unsigned char* const palette,
	bool* const out_textures_exist_flags )
{
	WallsTexturesNames walls_textures_names;
	LoadWallsTexturesNames( resources_file, walls_textures_names );

	glGenTextures( 1, &wall_textures_array_id_ );
	glBindTexture( GL_TEXTURE_2D_ARRAY, wall_textures_array_id_ );
	glTexImage3D(
		GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
		g_max_wall_texture_width, g_wall_texture_height, g_max_wall_textures,
		0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr );

	Vfs::FileContent texture_file;

	for( unsigned int t= 0u; t < g_max_wall_textures; t++ )
	{
		if( walls_textures_names[t][0] == '\0' )
		{
			out_textures_exist_flags[t]= false;
			continue;
		}
		out_textures_exist_flags[t]= true;

		vfs.ReadFile( walls_textures_names[t], texture_file );
		if( texture_file.empty() )
			continue;

		unsigned char texture_data[ g_max_wall_texture_width * g_wall_texture_height * 4u ];

		// TODO - check and discard incorrect textures
		unsigned short src_width, src_height;
		std::memcpy( &src_width , texture_file.data() + 0x2u, sizeof(unsigned short) );
		std::memcpy( &src_height, texture_file.data() + 0x4u, sizeof(unsigned short) );

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

void MapViewer::LoadModels(
	const Vfs& vfs,
	const Vfs::FileContent& resources_file,
	const unsigned char* palette )
{
	ModeslDescription models;
	const unsigned int model_count=
		LoadModelsNames( resources_file, models );

	Vfs::FileContent file_content;
	Vfs::FileContent animation_file_content;
	Model model;

	std::vector<unsigned  short> indeces;
	std::vector<Model::Vertex> vertices;

	// TODO - use atlas instead textures array
	const unsigned int c_max_texture_size[2]= { 64u, 1024u };
	const unsigned int c_texels_in_layer= c_max_texture_size[0u] * c_max_texture_size[1u];
	std::vector<unsigned char> textures_data_rgba( 4u * c_texels_in_layer * model_count, 0u );

	models_geometry_.resize( model_count );

	for( unsigned int m= 0u; m < model_count; m++ )
	{
		vfs.ReadFile( models[m].model_file_name, file_content );

		if( models[m].animation_file_name[0u] != '\0' )
			vfs.ReadFile( models[m].animation_file_name, animation_file_content );
		else
			animation_file_content.clear();

		LoadModel_o3( file_content, animation_file_content, model );

		if( model.texture_size[1u] > c_max_texture_size[1u] )
		{
			std::cout << "Model \"" << models[m].model_file_name << "\" texture height is too big: " << model.texture_size[1u] << std::endl;
			model.texture_size[1u]= c_max_texture_size[1u];
		}

		// Copy texture into atlas.
		unsigned char* const texture_dst= textures_data_rgba.data() + 4u * c_texels_in_layer * m;
		for( unsigned int y= 0u; y < model.texture_size[1u]; y++ )
		for( unsigned int x= 0u; x < model.texture_size[0u]; x++ )
		{
			const unsigned int i= ( x + y * c_max_texture_size[0u] ) << 2u;
			const unsigned char color_index= model.texture_data[ x + y * model.texture_size[0u] ];
			for( unsigned int j= 0u; j < 3u; j++ )
				texture_dst[ i + j ]= palette[ color_index * 3u + j ];
		}

		// Fill free texture space
		for(
			unsigned int y= model.texture_size[1u];
			y < ( ( c_max_texture_size[1u] + model.texture_size[1u] ) >> 1u );
			y++ )
		{
			std::memcpy(
				texture_dst + 4u * c_max_texture_size[0u] * y,
				texture_dst + 4u * c_max_texture_size[0u] * ( model.texture_size[1u] - 1u ),
				4u * c_max_texture_size[0u] );
		}
		for(
			unsigned int y= ( c_max_texture_size[1u] + model.texture_size[1u] )>> 1u;
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

		// Copy and recalculate indeces.
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

		models_geometry_[m].frame_count= model.frame_count;
		models_geometry_[m].vertex_count= model.vertices.size() / model.frame_count;
		models_geometry_[m].first_vertex_index= first_vertex_index;

		models_geometry_[m].first_index= first_index;
		models_geometry_[m].index_count= model.regular_triangles_indeces.size();

		models_geometry_[m].first_transparent_index= first_index + model.regular_triangles_indeces.size();
		models_geometry_[m].transparent_index_count= model.transparent_triangles_indeces.size();

	}

	// Prepare texture.
	glGenTextures( 1, &models_textures_array_id_ );
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

} // namespace ChasmReverse
