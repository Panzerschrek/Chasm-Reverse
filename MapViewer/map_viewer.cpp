#include "map_viewer.hpp"

namespace ChasmReverse
{

static const unsigned int g_floor_texture_size= 64u;
static const unsigned int g_floor_texture_texels= g_floor_texture_size * g_floor_texture_size;
static const unsigned int g_floor_textures_in_level= 64u;
static const unsigned int g_floors_file_header_size= 64u;
static const unsigned int g_bytes_per_floor_in_file= 86u * g_floor_texture_size;

MapViewer::MapViewer( const std::shared_ptr<Vfs>& vfs, unsigned int map_number )
{
	char map_file_name[16];

	const Vfs::FileContent palette= vfs->ReadFile( "CHASM2.PAL" );

	std::snprintf( map_file_name, sizeof(map_file_name), "FLOORS.%02u", map_number );
	const Vfs::FileContent floors_file= vfs->ReadFile( map_file_name );

	glGenTextures( 1, &floor_textures_array_id_ );
	glBindTexture( floor_textures_array_id_, GL_TEXTURE_2D_ARRAY );
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
}

MapViewer::~MapViewer()
{
	glDeleteTextures( 1, &floor_textures_array_id_ );
}

} // namespace ChasmReverse
