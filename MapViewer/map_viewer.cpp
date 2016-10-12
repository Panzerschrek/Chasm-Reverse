#include <cstring>

#include <shaders_loading.hpp>

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

typedef std::array<char[16], 256> WallsTexturesNames;

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

MapViewer::MapViewer( const std::shared_ptr<Vfs>& vfs, unsigned int map_number )
{
	char map_file_name[16];

	// Load floors textures
	const Vfs::FileContent palette= vfs->ReadFile( "CHASM2.PAL" );

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

	// Walls textures
	bool wall_texture_exist[ g_max_wall_textures ];
	std::snprintf( map_file_name, sizeof(map_file_name), "RESOURCE.%02u", map_number );
	LoadWallsTextures( *vfs, vfs->ReadFile( map_file_name ), palette.data(), wall_texture_exist );

	// Load walls geometry
	std::vector<WallVertex> walls_vertices;
	std::vector<unsigned short> walls_indeces;
	for( unsigned int y= 0u; y < g_map_size; y++ )
	for( unsigned int x= 1u; x < g_map_size; x++ )
	{
		const MapWall& map_wall=
			*reinterpret_cast<const MapWall*>( map_file.data() + 0x18001u + sizeof(MapWall) * ( x + y * g_map_size ) );

		if( map_wall.texture_id >= 128u ||
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
		models_shader_.Create();
	}

	// Test model
	Model model;
	LoadModel( vfs->ReadFile( "table1.3o" ), model );

	{
		std::vector<unsigned char> texture_rgba( model.texture_data.size() * 4u );

		for( unsigned int i= 0u; i < model.texture_data.size(); i++ )
		{
			const unsigned char color_index= model.texture_data[i];
			for( unsigned int j= 0u; j < 3u; j++ )
				texture_rgba[ i * 4u + j ]= palette[ color_index * 3u + j ] << 2u;
		}

		test_model_texture_=
			r_Texture(
				r_Texture::PixelFormat::RGBA8,
				model.texture_size[0], model.texture_size[1],
				texture_rgba.data() );

		test_model_texture_.SetFiltration( r_Texture::Filtration::NearestMipmapLinear, r_Texture::Filtration::Nearest );
		test_model_texture_.BuildMips();

		test_model_geometry_.VertexData(
			model.vertices.data(),
			model.vertices.size() * sizeof(Model::Vertex),
			sizeof(Model::Vertex) );

		test_model_geometry_.IndexData(
			model.regular_triangles_indeces.data(),
			model.regular_triangles_indeces.size() * sizeof(unsigned short),
			GL_UNSIGNED_SHORT,
			GL_TRIANGLES );

		Model::Vertex v;
		test_model_geometry_.VertexAttribPointer(
			0, 3, GL_FLOAT, false,
			((char*)v.pos) - ((char*)&v) );

		test_model_geometry_.VertexAttribPointer(
			1, 3, GL_FLOAT, false,
			((char*)&v.tex_coord) - ((char*)&v) );
	}
}

MapViewer::~MapViewer()
{
	glDeleteTextures( 1, &floor_textures_array_id_ );
	glDeleteTextures( 1, &wall_textures_array_id_ );
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
	test_model_texture_.Bind(0);
	models_shader_.Bind();

	models_shader_.Uniform( "tex", int(0) );
	models_shader_.Uniform( "view_matrix", view_matrix );

	test_model_geometry_.Draw();
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
				texture_data[ (i << 2u) + j ]= palette[ color_index * 3u + j ] << 2u;
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
					texture_data[ i + j ]= palette[ color_index * 3u + j ] << 2;

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

} // namespace ChasmReverse
