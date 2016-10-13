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
			Translucent= (0x04u | 0x08u),
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
static const float g_3o_model_coords_scale= 1.0f / 2048.0f;

void LoadModel_o3( const Vfs::FileContent& model_file, const Vfs::FileContent& animation_file, Model& out_model )
{
	// Clear output
	out_model.vertices.clear();
	out_model.regular_triangles_indeces.clear();
	out_model.transparent_triangles_indeces.clear();
	out_model.texture_data.clear();

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

	const unsigned char* const in_vertices_data=
		animation_file.empty()
			? ( model_file.data() + 0x3200u )
			: ( animation_file.data() + 0x02u );
	const Vertex_o3* const vertices= reinterpret_cast<const Vertex_o3*>( in_vertices_data );

	out_model.frame_count=
		animation_file.empty()
			? 1u
			: ( animation_file.size() - 2u ) / ( vertex_count * sizeof(Vertex_o3) );

	std::vector<Model::Vertex> tmp_vertices;
	unsigned int current_vertex_index= 0u;

	for( unsigned int p= 0u; p < polygon_count; p++ )
	{
		const Polygon_o3& polygon= polygons[p];
		const bool polygon_is_triangle= polygon.vertices_indeces[3u] >= vertex_count;
		const bool polygon_is_twosided= ( polygon.flags & Polygon_o3::Flags::Twosided ) != 0u;

		const unsigned int polygon_vertex_count= polygon_is_triangle ? 3u : 4u;
		unsigned int polygon_index_count= polygon_is_triangle ? 3u : 6u;
		if( polygon_is_twosided ) polygon_index_count*= 2u;

		const unsigned int first_vertex_index= current_vertex_index;
		tmp_vertices.resize( tmp_vertices.size() + polygon_vertex_count * out_model.frame_count );
		Model::Vertex* v= tmp_vertices.data() + first_vertex_index * out_model.frame_count;

		for( unsigned int j= 0u; j < polygon_vertex_count; j++ )
		{
			for( unsigned int frame= 0u; frame < out_model.frame_count; frame++ )
			{
				Model::Vertex& vertex= v[ frame + j * out_model.frame_count ];
				const Vertex_o3& in_vertex= vertices[ polygon.vertices_indeces[j] + frame * vertex_count ];

				vertex.tex_coord[0]= float( polygon.uv[j][0] + 1u ) / float( out_model.texture_size[0] );
				vertex.tex_coord[1]= float( polygon.uv[j][1] + polygon.v_offset ) / float( out_model.texture_size[1] );

				for( unsigned int c= 0u; c < 3u; c++ )
					vertex.pos[c]= float( in_vertex.xyz[c] ) * g_3o_model_coords_scale;
			}
		}

		auto& dst_indeces=
			(polygon.flags & Polygon_o3::Flags::Translucent ) == 0u
				? out_model.regular_triangles_indeces
				: out_model.transparent_triangles_indeces;

		dst_indeces.resize( dst_indeces.size() + polygon_index_count );
		unsigned short* ind= dst_indeces.data() + dst_indeces.size() - polygon_index_count;

		ind[0u]= first_vertex_index + 2u;
		ind[1u]= first_vertex_index + 1u;
		ind[2u]= first_vertex_index + 0u;
		if( !polygon_is_triangle )
		{
			ind[3u]= first_vertex_index + 0u;
			ind[4u]= first_vertex_index + 3u;
			ind[5u]= first_vertex_index + 2u;
		}

		if( polygon_is_twosided )
		{
			ind+= polygon_index_count >> 1u;
			ind[0u]= first_vertex_index + 0u;
			ind[1u]= first_vertex_index + 1u;
			ind[2u]= first_vertex_index + 2u;
			if( !polygon_is_triangle )
			{
				ind[3u]= first_vertex_index + 2u;
				ind[4u]= first_vertex_index + 3u;
				ind[5u]= first_vertex_index + 0u;
			}
		}

		current_vertex_index+= polygon_vertex_count;
	} // for polygons

	// Change vertices frames order.
	out_model.vertices.resize( tmp_vertices.size() );
	const unsigned int out_vertex_count= tmp_vertices.size() / out_model.frame_count;

	for( unsigned int frame= 0u; frame < out_model.frame_count; frame++ )
	{
		for( unsigned int v= 0u; v < out_vertex_count; v++ )
			out_model.vertices[ frame * out_vertex_count + v ]=
				tmp_vertices[ v * out_model.frame_count + frame ];
	}
}

} // namespace ChasmReverse
