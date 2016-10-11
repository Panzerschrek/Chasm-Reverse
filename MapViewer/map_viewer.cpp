#include <shaders_loading.hpp>

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

struct FloorVertex
{
	unsigned char xy[2];
	unsigned char texture_id;
	unsigned char reserved;
};

static_assert( sizeof(FloorVertex) == 4u, "Invalid size" );

MapViewer::MapViewer( const std::shared_ptr<Vfs>& vfs, unsigned int map_number )
{
	char map_file_name[16];

	// Load floors textures
	const Vfs::FileContent palette= vfs->ReadFile( "CHASM2.PAL" );

	std::snprintf( map_file_name, sizeof(map_file_name), "FLOORS.%02u", map_number );
	const Vfs::FileContent floors_file= vfs->ReadFile( map_file_name );

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

		for( unsigned int i= 0; i < g_floor_texture_texels; i++ )
		{
			const unsigned char color_index= in_data[i];
			for( unsigned int j= 0; j < 3u; j++ )
				texture_data[ (i << 2u) + j ]= palette[ color_index * 3u + j ] << 2u;
			texture_data[ (i << 2u) + 3 ]= 255;
		}

		glTexSubImage3D(
			GL_TEXTURE_2D_ARRAY,
			0, 0, 0,
			t, g_floor_texture_size, g_floor_texture_size,
			1, GL_RGBA, GL_UNSIGNED_BYTE,
			texture_data );

	} // for textures

	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
	glGenerateMipmap( GL_TEXTURE_2D_ARRAY );

	// Load floors geometry
	std::snprintf( map_file_name, sizeof(map_file_name), "MAP.%02u", map_number );
	const Vfs::FileContent map_file= vfs->ReadFile( map_file_name );

	std::vector<FloorVertex> floors_vertices;
	for( unsigned int floor_or_ceiling= 0u; floor_or_ceiling < 2u; floor_or_ceiling++ )
	{
		floors_geometry_info[ floor_or_ceiling ].first_vertex_number= floors_vertices.size();


		const unsigned char* const in_data= map_file.data() + 0x23001u + g_map_cells * floor_or_ceiling;

		for( unsigned int y= 0u; y < g_map_size; y++ )
		for( unsigned int x= 0u; x < g_map_size; x++ )
		{
			unsigned char texture_number= in_data[ y * g_map_size + x ];
			if( texture_number == 0u )
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

	// Shaders
	const r_GLSLVersion glsl_version( r_GLSLVersion::KnowmNumbers::v330, r_GLSLVersion::Profile::Core );

	floors_shader_.ShaderSource(
		rLoadShader( "floors_f.glsl", glsl_version ),
		rLoadShader( "floors_v.glsl", glsl_version ) );
	floors_shader_.SetAttribLocation( "pos", 0u );
	floors_shader_.SetAttribLocation( "tex_id", 1u );
	floors_shader_.Create();
}

MapViewer::~MapViewer()
{
	glDeleteTextures( 1, &floor_textures_array_id_ );
}

void MapViewer::Draw( const m_Mat4& view_matrix )
{
	floors_geometry_.Bind();

	floors_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, floor_textures_array_id_ );

	floors_shader_.Uniform( "tex", int(0) );

	floors_shader_.Uniform( "view_matrix", view_matrix );

	// Draw floors and ceilings
	for( unsigned int z= 0; z < 2; z++ )
	{
		floors_shader_.Uniform( "pos_z", float( 1u - z ) * 2.0f );

		glDrawArrays(
			GL_TRIANGLES,
			floors_geometry_info[z].first_vertex_number,
			floors_geometry_info[z].vertex_count );
	}
}

} // namespace ChasmReverse
