#include <cstring>

#include "model.hpp"

namespace ChasmReverse
{


struct Polygon_o3
{
	unsigned short vertices_indeces[4u];
	unsigned short uv[4u][2u];
	unsigned char unknown0[4];
	unsigned char unknown1;
	unsigned char flags;
	unsigned short v_offset;

	struct Flags
	{
		enum : unsigned int
		{
			Twosided= 0x01u,
			Translucent= 0x08u,
		};
	};
};

static_assert( sizeof(Polygon_o3) == 32u, "Invalid size" );

struct Vertex_o3
{
	short xyz[3u];
};

static_assert( sizeof(Vertex_o3) == 6u, "Invalid size" );

static const unsigned int g_3o_model_texture_width= 64u;

void LoadModel( const Vfs::FileContent& model_file, Model& out_model )
{
	// Clear output
	out_model.vertices.clear();
	out_model.regular_triangles_indeces.clear();
	out_model.transparent_triangles_indeces.clear();

	unsigned short polygon_count;
	unsigned short vertex_count;
	unsigned short texture_height;

	std::memcpy( &vertex_count  , model_file.data() + 0x4800u, sizeof(unsigned short ) );
	std::memcpy( &polygon_count , model_file.data() + 0x4802u, sizeof(unsigned short ) );
	std::memcpy( &texture_height, model_file.data() + 0x4804u, sizeof(unsigned short ) );

	// Texture
	out_model.texture_size[0]= g_3o_model_texture_width;
	out_model.texture_size[1]= texture_height;

	out_model.texture_data.resize( g_3o_model_texture_width * texture_height );
	std::memcpy(
		out_model.texture_data.data(),
		model_file.data() + 0x4806u,
		out_model.texture_data.size() );

	// Geometry
	const Polygon_o3* const polygons= reinterpret_cast<const Polygon_o3*>( model_file.data() + 0x00u );
	const Vertex_o3* const vertices= reinterpret_cast<const Vertex_o3*>( model_file.data() + 0x3200u );

	for( unsigned int p= 0u; p < polygon_count; p++ )
	{
		const Polygon_o3& polygon= polygons[p];

		const unsigned int first_vertex_index= out_model.vertices.size();
		out_model.vertices.resize( out_model.vertices.size() + 4u );
		Model::Vertex* v= out_model.vertices.data() + first_vertex_index;

		for( unsigned int j= 0u; j < 4u; j++ )
		{
			v[j].tex_coord[0]= float( polygon.uv[j][0] ) / float( out_model.texture_size[0] );
			v[j].tex_coord[1]= float( polygon.uv[j][1] + polygon.v_offset ) / float( out_model.texture_size[1] );
			for( unsigned int c= 0u; c < 3u; c++ )
				v[j].pos[c]= float( vertices[ polygon.vertices_indeces[j] ].xyz[c] ) / 256.0f;
		}

		const unsigned int index_count= polygon.vertices_indeces[3u] >= vertex_count ? 3u : 6u;

		auto& dst_indeces= out_model.regular_triangles_indeces;
		dst_indeces.resize( dst_indeces.size() + index_count );
		unsigned short* const ind= dst_indeces.data() + dst_indeces.size() - index_count;

		ind[0u]= first_vertex_index + 0u;
		ind[1u]= first_vertex_index + 1u;
		ind[2u]= first_vertex_index + 2u;
		if( index_count == 6u )
		{
			ind[3u]= first_vertex_index + 0u;
			ind[4u]= first_vertex_index + 2u;
			ind[5u]= first_vertex_index + 3u;
		}
	} // for polygons
}

} // namespace ChasmReverse
