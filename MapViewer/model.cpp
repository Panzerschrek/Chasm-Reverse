#include <cstring>
#include <iostream>

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

struct CARHeader
{
	unsigned short animations[20u];
	unsigned short submodels_animations[3u][2u];
	unsigned short unknown[25u];
};

static_assert( sizeof(CARHeader) == 0x66u, "Invalid size" );

static const unsigned int g_3o_model_texture_width= 64u;
static const float g_3o_model_coords_scale= 1.0f / 2048.0f;

static const unsigned int g_car_model_texture_width= 64u;

// Swap from order
// v0.f0 v0.f1 v0.f2  v1.f0 v1.f1 v1.f2
// to
// v0.f0 v1.f0  v0.f1 v1.f1  v0.f2 v1.f2
static void SwapVertexOrder(
	Model::Vertices& in_vertices,
	const unsigned int frame_count,
	Model::Vertices& out_vertices )
{
	out_vertices.resize( in_vertices.size() );

	const unsigned int out_vertex_count= in_vertices.size() / frame_count;

	for( unsigned int frame= 0u; frame < frame_count; frame++ )
	{
		for( unsigned int v= 0u; v < out_vertex_count; v++ )
			out_vertices[ frame * out_vertex_count + v ]=
				in_vertices[ v * frame_count + frame ];
	}
}


void LoadModel_o3( const Vfs::FileContent& model_file, const Vfs::FileContent& animation_file, Model& out_model )
{
	// Clear output
	out_model.vertices.clear();
	out_model.regular_triangles_indeces.clear();
	out_model.transparent_triangles_indeces.clear();
	out_model.texture_data.clear();
	out_model.submodels.clear();

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

	SwapVertexOrder( tmp_vertices, out_model.frame_count, out_model.vertices );

	// Setup animation
	out_model.animations.resize( 1u );
	Model::Animation& anim= out_model.animations.back();

	anim.id= 0u;
	anim.first_frame= out_model.frame_count;
	anim.frame_count= out_model.frame_count;
}

void LoadModel_car( const Vfs::FileContent& model_file, Model& out_model )
{
	const unsigned int c_textures_offset= 0x486Cu;

	unsigned short vertex_count;
	unsigned short polygon_count;
	unsigned short texture_texels;
	std::memcpy( &vertex_count, model_file.data() + 0x4866u, sizeof(unsigned short) );
	std::memcpy( &polygon_count, model_file.data() + 0x4868u, sizeof(unsigned short) );
	std::memcpy( &texture_texels, model_file.data() + 0x486Au, sizeof(unsigned short) );

	out_model.texture_size[0u]= g_car_model_texture_width;
	out_model.texture_size[1u]= texture_texels / g_car_model_texture_width;

	out_model.texture_data.resize( texture_texels );
	std::memcpy(
		out_model.texture_data.data(),
		model_file.data() + c_textures_offset,
		texture_texels );

	const CARHeader* const header= reinterpret_cast<const CARHeader*>( model_file.data() );

	out_model.frame_count= 0u;
	for( unsigned int i= 0u; i < 20u; i++ )
	{
		const unsigned int animation_frame_count= header->animations[i] / ( sizeof(Vertex_o3) * vertex_count );
		if( animation_frame_count == 0u ) continue;

		out_model.animations.emplace_back();
		Model::Animation& anim= out_model.animations.back();

		anim.id= i;
		anim.first_frame= out_model.frame_count;
		anim.frame_count= animation_frame_count;

		out_model.frame_count+= animation_frame_count;
	}

	const auto prepare_model=
	[&out_model](
		const unsigned int vertex_count, const unsigned int polygon_count,
		const Vertex_o3* const vertices, const Polygon_o3* const polygons,
		Submodel& out_submodel )
	{
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
			tmp_vertices.resize( tmp_vertices.size() + polygon_vertex_count * out_submodel.frame_count );
			Model::Vertex* v= tmp_vertices.data() + first_vertex_index * out_submodel.frame_count;

			for( unsigned int j= 0u; j < polygon_vertex_count; j++ )
			{
				for( unsigned int frame= 0u; frame < out_submodel.frame_count; frame++ )
				{
					Model::Vertex& vertex= v[ frame + j * out_submodel.frame_count ];
					const Vertex_o3& in_vertex= vertices[ polygon.vertices_indeces[j] + frame * vertex_count ];

					vertex.tex_coord[0]= float( polygon.uv[j][0]) / float( out_model.texture_size[0] << 8u );
					vertex.tex_coord[1]= float( polygon.uv[j][1] + 4u * polygon.v_offset ) / float( out_model.texture_size[1] << 8u );

					for( unsigned int c= 0u; c < 3u; c++ )
						vertex.pos[c]= float( in_vertex.xyz[c] ) * g_3o_model_coords_scale;
				}
			}

			auto& dst_indeces=
				(polygon.flags & Polygon_o3::Flags::Translucent ) == 0u
					? out_submodel.regular_triangles_indeces
					: out_submodel.transparent_triangles_indeces;

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

		SwapVertexOrder( tmp_vertices, out_submodel.frame_count, out_submodel.vertices );
	};

	{ // Main model
		const Vertex_o3* vertices=
			reinterpret_cast<const Vertex_o3*>( model_file.data() + c_textures_offset + texture_texels );
		const Polygon_o3* const polygons=
			reinterpret_cast<const Polygon_o3*>( model_file.data() + 0x66u );

		prepare_model( vertex_count, polygon_count, vertices, polygons, out_model );
	}

	unsigned int submodels_offset=
		c_textures_offset +
		out_model.texture_data.size() +
		out_model.frame_count * sizeof(Vertex_o3) * vertex_count;

	for( unsigned int i= 0u; i < 3u; i++ )
	{
		const unsigned int c_animation_data_offset= 0x4806u;

		const unsigned int submodel_animation_data_size=
			header->submodels_animations[i][0u] + header->submodels_animations[i][1u];

		if( submodel_animation_data_size == 0u )
			continue;

		out_model.submodels.emplace_back();
		Submodel& submodel= out_model.submodels.back();

		std::memcpy( &vertex_count, model_file.data() + submodels_offset + 0x4800u, sizeof(unsigned short) );
		std::memcpy( &polygon_count, model_file.data() + submodels_offset + 0x4802u, sizeof(unsigned short) );

		submodel.frame_count= submodel_animation_data_size / ( sizeof(Vertex_o3) * vertex_count );

		const Vertex_o3* vertices=
			reinterpret_cast<const Vertex_o3*>( model_file.data() + submodels_offset + c_animation_data_offset );
		const Polygon_o3* const polygons=
			reinterpret_cast<const Polygon_o3*>( model_file.data() + submodels_offset );

		prepare_model( vertex_count, polygon_count, vertices, polygons, submodel );

		// Setup animations.
		// Each submodel have up to 2 animations.
		for( unsigned int a= 0u; a < 2u; a++ )
		{
			const unsigned int animation_frame_count=
				header->submodels_animations[i][a] / ( sizeof(Vertex_o3) * vertex_count );
			if( animation_frame_count == 0u )
				continue;

			submodel.animations.emplace_back();
			Submodel::Animation& anim= submodel.animations.back();

			anim.id= i;
			anim.first_frame= submodel.frame_count;
			anim.frame_count= animation_frame_count;
		}

		submodels_offset+= c_animation_data_offset + submodel_animation_data_size;
	} // for submodels
}

} // namespace ChasmReverse
