#pragma once
#include <vector>
#include <bbox.hpp>
#include "assert.hpp"
#include "vfs.hpp"

namespace PanzerChasm
{

struct Submodel
{
	// All vertex structures are in GPU-friendly format.

	struct Vertex
	{
		float tex_coord[2];
		unsigned short vertex_id; // Id of anmiation vertex
		unsigned char texture_id;
		unsigned char alpha_test_mask; // Zero for regular polygons, 255 - for alpha-tested
		unsigned char groups_mask; // Mask for groups. Valid only for .car models.

		unsigned char reserved[3];
	};

	SIZE_ASSERT( Vertex, 16u );

	struct AnimationVertex
	{
		short pos[3];
		short reserved[1];
	};

	SIZE_ASSERT( AnimationVertex, 8u );

	typedef std::vector<Vertex> Vertices;
	typedef std::vector<AnimationVertex> AnimationsVertices;

	struct Animation
	{
		unsigned int id; // animation id from original file
		unsigned int first_frame;
		unsigned int frame_count;
	};

	unsigned int frame_count;
	std::vector<Animation> animations;
	Vertices vertices;
	std::vector<AnimationVertex> animations_vertices;
	std::vector<unsigned short> regular_triangles_indeces;
	std::vector<unsigned short> transparent_triangles_indeces;

	// Store separate bounding box for each frame for better culling.
	std::vector<m_BBox3> animations_bboxes;

	// Associated with models sounds (raw PCM)
	std::vector< std::vector<unsigned char> > sounds;

	float z_min, z_max;
};

struct Model final : public Submodel
{
	unsigned int texture_size[2];
	std::vector<unsigned char> texture_data;

	std::vector<Submodel> submodels;
};

void LoadModel_o3( const Vfs::FileContent& model_file, const Vfs::FileContent& animation_file, Model& out_model );
void LoadModel_o3(
	const Vfs::FileContent& model_file,
	const Vfs::FileContent* animation_files, unsigned int animation_files_count,
	Model& out_model );

void LoadModel_car( const Vfs::FileContent& model_file, Model& out_model );
void LoadModel_gltf( const Vfs::FileContent& model_file, Model& out_model );

} // namespace ChasmReverse
