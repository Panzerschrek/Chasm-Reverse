#include <cstring>

#include "assert.hpp"
#include "math_utils.hpp"

#include "model.hpp"

namespace PanzerChasm
{

struct Polygon_o3
{
	unsigned short vertices_indeces[4u];
	unsigned short uv[4u][2u];
	unsigned char unknown0[4];
	unsigned char group_id; // For monsters - body, left hand, right hand, head, etc.
	unsigned char flags;
	unsigned short v_offset;

	struct Flags
	{
		enum : unsigned int
		{
			Twosided= 0x01u,
			AlphaTested= 0x02u,
			Translucent= (0x04u | 0x08u),
		};
	};
};

SIZE_ASSERT( Polygon_o3, 32 );

struct Vertex_o3
{
	short xyz[3u];
};

SIZE_ASSERT( Vertex_o3, 6 );

struct CARHeader
{
	static constexpr unsigned int c_sound_count= 7u;

	unsigned short animations[20u];
	unsigned short submodels_animations[3u][2u];

	// 6, 7, 8 - gibs type
	unsigned short unknown0[9];

	// Values - sound data size, in bytes.
	unsigned short sounds[c_sound_count];

	// 0 - always zero, maybe
	// 1 - 8  values, like 64, 96, 128 etc.
	unsigned short unknown1[9];
};

SIZE_ASSERT( CARHeader, 0x66 );

static const unsigned int g_3o_model_texture_width= 64u;
static const float g_3o_model_coords_scale= 1.0f / 2048.0f;

static const unsigned int g_car_model_texture_width= 64u;

static void ClearModel( Model& model )
{
	model.animations.clear();
	model.vertices.clear();
	model.animations_vertices.clear();
	model.regular_triangles_indeces.clear();
	model.transparent_triangles_indeces.clear();
	model.texture_data.clear();
	model.submodels.clear();
}

static void CalculateModelZ( Model& model, const Vertex_o3* const vertices, const unsigned int vertex_count )
{
	model.z_max= Constants::min_float;
	model.z_min= Constants::max_float;
	for( unsigned int v= 0u; v < vertex_count; v++ )
	{
		const float z= float( vertices[v].xyz[2] ) * g_3o_model_coords_scale;
		model.z_max= std::max( model.z_max, z );
		model.z_min= std::min( model.z_min, z );
	}
}

static void CalculateBoundingBox(
	const Vertex_o3* const vertices, unsigned int vertex_count,
	m_BBox3& out_bbox )
{
	out_bbox.min.x= out_bbox.min.y= out_bbox.min.z= Constants::max_float;
	out_bbox.max.x= out_bbox.max.y= out_bbox.max.z= Constants::min_float;

	for( unsigned int i= 0u; i < vertex_count; i++ )
		out_bbox +=
			m_Vec3(
				float( vertices[i].xyz[0] ),
				float( vertices[i].xyz[1] ),
				float( vertices[i].xyz[2] ) ) * g_3o_model_coords_scale;
}

static unsigned char GroupIdToGroupsMask( const unsigned char group_id )
{
	// 64 is unsused. Map to it "zero".
	return group_id == 0 ? 64u : group_id;
}

void LoadModel_o3( const Vfs::FileContent& model_file, const Vfs::FileContent& animation_file, Model& out_model )
{
	ClearModel( out_model );

	unsigned short polygon_count;
	unsigned short vertex_count;
	unsigned short texture_height;

	std::memcpy( &vertex_count  , model_file.data() + 0x4800u, sizeof(unsigned short ) );
	std::memcpy( &polygon_count , model_file.data() + 0x4802u, sizeof(unsigned short ) );
	std::memcpy( &texture_height, model_file.data() + 0x4804u, sizeof(unsigned short ) );

	const unsigned int v_offset_shift= texture_height & ~1023u;
	texture_height&= 1023u;

	// Texture
	out_model.texture_size[0]= g_3o_model_texture_width;
	out_model.texture_size[1]= texture_height;

	out_model.texture_data.resize( g_3o_model_texture_width * texture_height );
	std::memcpy(
		out_model.texture_data.data(),
		model_file.data() + 0x4806u,
		out_model.texture_data.size() );


	PC_ASSERT( model_file.size() == out_model.texture_data.size() + 0x4806u );

	// Geometry
	const Polygon_o3* const polygons= reinterpret_cast<const Polygon_o3*>( model_file.data() + 0x00u );

	const unsigned char* const in_vertices_data=
		animation_file.empty()
			? ( model_file.data() + 0x3200u )
			: ( animation_file.data() + 0x02u );
	const Vertex_o3* const vertices= reinterpret_cast<const Vertex_o3*>( in_vertices_data );

	CalculateModelZ(
		out_model,
		reinterpret_cast<const Vertex_o3*>( model_file.data() + 0x3200u ),
		vertex_count );

	out_model.frame_count=
		animation_file.empty()
			? 1u
			: ( animation_file.size() - 2u ) / ( vertex_count * sizeof(Vertex_o3) );

	out_model.animations_vertices.resize( out_model.frame_count * vertex_count );
	for( unsigned int v= 0u; v < out_model.animations_vertices.size(); v++ )
	{
		for( unsigned int j= 0u; j < 3; j++ )
			out_model.animations_vertices[v].pos[j]= vertices[v].xyz[j];
	}

	unsigned int current_vertex_index= 0u;

	for( unsigned int p= 0u; p < polygon_count; p++ )
	{
		const Polygon_o3& polygon= polygons[p];
		const bool polygon_is_triangle= polygon.vertices_indeces[3u] >= vertex_count;
		const bool polygon_is_twosided= ( polygon.flags & Polygon_o3::Flags::Twosided ) != 0u;

		const bool transparent= ( polygon.flags & Polygon_o3::Flags::Translucent ) != 0;
		const unsigned char alpha_test_mask= ( transparent || ( (polygon.flags & Polygon_o3::Flags::AlphaTested) != 0 ) ) ? 255 : 0;

		const unsigned int polygon_vertex_count= polygon_is_triangle ? 3u : 4u;
		unsigned int polygon_index_count= polygon_is_triangle ? 3u : 6u;
		if( polygon_is_twosided ) polygon_index_count*= 2u;

		const unsigned int first_vertex_index= current_vertex_index;
		out_model.vertices.resize( out_model.vertices.size() + polygon_vertex_count );
		Model::Vertex* v= out_model.vertices.data() + first_vertex_index;

		const unsigned int v_offset= polygon.v_offset + v_offset_shift;
		for( unsigned int j= 0u; j < polygon_vertex_count; j++ )
		{
			Model::Vertex& vertex= v[j];

			vertex.tex_coord[0]= float( polygon.uv[j][0] ) / float( out_model.texture_size[0] );
			vertex.tex_coord[1]= float( polygon.uv[j][1] + v_offset ) / float( out_model.texture_size[1] );
			vertex.vertex_id= polygon.vertices_indeces[j];

			vertex.alpha_test_mask= alpha_test_mask;
			vertex.groups_mask= 255u;
		}

		auto& dst_indeces=
			transparent
				? out_model.transparent_triangles_indeces
				: out_model.regular_triangles_indeces;

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

	// Setup animation
	out_model.animations.resize( 1u );
	Model::Animation& anim= out_model.animations.back();

	anim.id= 0u;
	anim.first_frame= 0u;
	anim.frame_count= out_model.frame_count;

	// Calculate bounding boxes.
	out_model.animations_bboxes.resize( out_model.frame_count );
	for( unsigned int i= 0u; i < out_model.frame_count; i++ )
		CalculateBoundingBox( vertices + i * vertex_count, vertex_count, out_model.animations_bboxes[i] );
}

void LoadModel_o3(
	const Vfs::FileContent& model_file,
	const Vfs::FileContent* const animation_files, const unsigned int animation_files_count,
	Model& out_model )
{
	constexpr unsigned int c_max_animations= 32;

	PC_ASSERT( animation_files != nullptr );
	PC_ASSERT( animation_files_count >= 1u );
	PC_ASSERT( animation_files_count < c_max_animations );

	Model::Animation animations[ c_max_animations ];

	unsigned int frame_count= 0u;
	unsigned short vertex_count= 0u;

	for( unsigned int i= 0; i < animation_files_count; i++ )
	{
		PC_ASSERT( !animation_files[i].empty() );

		std::memcpy( &vertex_count, animation_files[i].data(), sizeof(unsigned short) );

		animations[i].id= i;
		animations[i].first_frame= frame_count;
		animations[i].frame_count= ( animation_files[i].size() - 2u ) / ( vertex_count * sizeof(Vertex_o3) );

		frame_count+= animations[i].frame_count;
	}

	// Produce big animation file
	Vfs::FileContent combined_animations( sizeof(unsigned short) + vertex_count * frame_count * sizeof(Vertex_o3) );

	std::memcpy( combined_animations.data(), &frame_count, sizeof(unsigned short) );
	unsigned char* ptr= combined_animations.data() + sizeof(unsigned short);

	for( unsigned int i= 0; i < animation_files_count; i++ )
	{
		const unsigned int animation_data_size= animation_files[i].size() - sizeof(unsigned short);
		std::memcpy( ptr, animation_files[i].data() + sizeof(unsigned short), animation_data_size );
		ptr+= animation_data_size;
	}

	LoadModel_o3( model_file, combined_animations, out_model );

	out_model.animations.resize( animation_files_count );
	std::memcpy( out_model.animations.data(), animations, sizeof(Model::Animation) * animation_files_count );
}

void LoadModel_car( const Vfs::FileContent& model_file, Model& out_model )
{
	ClearModel( out_model );

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
		out_submodel.animations_vertices.resize( out_submodel.frame_count * vertex_count );
		for( unsigned int v= 0u; v < out_submodel.animations_vertices.size(); v++ )
		{
			for( unsigned int j= 0u; j < 3; j++ )
				out_submodel.animations_vertices[v].pos[j]= vertices[v].xyz[j];
		}

		unsigned int current_vertex_index= 0u;

		for( unsigned int p= 0u; p < polygon_count; p++ )
		{
			const Polygon_o3& polygon= polygons[p];
			const bool polygon_is_triangle= polygon.vertices_indeces[3u] >= vertex_count;
			const bool polygon_is_twosided= ( polygon.flags & Polygon_o3::Flags::Twosided ) != 0u;

			const bool transparent= ( polygon.flags & Polygon_o3::Flags::Translucent ) != 0;
			const unsigned char alpha_test_mask= ( transparent || ( (polygon.flags & Polygon_o3::Flags::AlphaTested) != 0 ) ) ? 255 : 0;
			const unsigned char groups_mask= GroupIdToGroupsMask( polygon.group_id );

			const unsigned int polygon_vertex_count= polygon_is_triangle ? 3u : 4u;
			unsigned int polygon_index_count= polygon_is_triangle ? 3u : 6u;
			if( polygon_is_twosided ) polygon_index_count*= 2u;

			const unsigned int first_vertex_index= current_vertex_index;
			out_submodel.vertices.resize( out_submodel.vertices.size() + polygon_vertex_count );
			Model::Vertex* v= out_submodel.vertices.data() + first_vertex_index;

			for( unsigned int j= 0u; j < polygon_vertex_count; j++ )
			{
				Model::Vertex& vertex= v[j];

				vertex.tex_coord[0]= float( polygon.uv[j][0]) / float( out_model.texture_size[0] << 8u );
				vertex.tex_coord[1]= float( polygon.uv[j][1] + 4u * polygon.v_offset ) / float( out_model.texture_size[1] << 8u );
				vertex.vertex_id= polygon.vertices_indeces[j];
				vertex.alpha_test_mask= alpha_test_mask;
				vertex.groups_mask= groups_mask;
			}

			auto& dst_indeces=
				transparent
					? out_submodel.transparent_triangles_indeces
					: out_submodel.regular_triangles_indeces;

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

		// Calculate bounding boxes.
		out_submodel.animations_bboxes.resize( out_submodel.frame_count );
		for( unsigned int i= 0u; i < out_submodel.frame_count; i++ )
			CalculateBoundingBox( vertices + i * vertex_count, vertex_count, out_submodel.animations_bboxes[i] );
	};

	{ // Main model
		const Vertex_o3* vertices=
			reinterpret_cast<const Vertex_o3*>( model_file.data() + c_textures_offset + texture_texels );
		const Polygon_o3* const polygons=
			reinterpret_cast<const Polygon_o3*>( model_file.data() + 0x66u );

		prepare_model( vertex_count, polygon_count, vertices, polygons, out_model );

		// For all "Car" models first animation frame is frame of "run" animation
		CalculateModelZ( out_model, vertices, vertex_count );
	}

	unsigned int submodels_offset=
		c_textures_offset +
		out_model.texture_data.size() +
		out_model.frame_count * sizeof(Vertex_o3) * vertex_count;

	out_model.submodels.resize(3u);
	for( Submodel& submodel : out_model.submodels )
	{
		submodel.frame_count= 0u;
		submodel.z_min= submodel.z_max= 0.0f;
	}

	for( unsigned int i= 0u; i < 3u; i++ )
	{
		const unsigned int c_animation_data_offset= 0x4806u;

		const unsigned int submodel_animation_data_size=
			header->submodels_animations[i][0u] + header->submodels_animations[i][1u];

		if( submodel_animation_data_size == 0u )
			continue;

		Submodel& submodel= out_model.submodels[i];

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
		unsigned int first_submodel_animation_frame= 0u;
		for( unsigned int a= 0u; a < 2u; a++ )
		{
			const unsigned int animation_frame_count=
				header->submodels_animations[i][a] / ( sizeof(Vertex_o3) * vertex_count );
			if( animation_frame_count == 0u )
				continue;

			submodel.animations.emplace_back();
			Submodel::Animation& anim= submodel.animations.back();

			anim.id= i;
			anim.first_frame= first_submodel_animation_frame;
			anim.frame_count= animation_frame_count;
			first_submodel_animation_frame+= anim.frame_count;
		}

		submodels_offset+= c_animation_data_offset + submodel_animation_data_size;
	} // for submodels

	unsigned int sounds_offset= submodels_offset;

	out_model.sounds.resize( CARHeader::c_sound_count );
	for( unsigned int i= 0u; i < CARHeader::c_sound_count; i++ )
	{
		std::vector<unsigned char>& sound= out_model.sounds[i];
		sound.resize( header->sounds[i] );
		std::memcpy( sound.data(), model_file.data() + sounds_offset, header->sounds[i] );

		sounds_offset+= header->sounds[i];
	}

	PC_ASSERT( sounds_offset == model_file.size() );
}

void LoadModel_gltf( const Vfs::FileContent& model_file, Model& out_model )
{
  ClearModel( out_model );
}

} // namespace ChasmReverse
