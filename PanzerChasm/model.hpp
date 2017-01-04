#pragma once
#include <vector>

#include "vfs.hpp"

namespace PanzerChasm
{

struct Submodel
{
	struct Vertex
	{
		float pos[3];
		float tex_coord[2];
		unsigned char texture_id;
		unsigned char alpha_test_mask; // Zero for regular polygons, 255 - for alpha-tested
		unsigned char groups_mask; // Mask for groups. Valid only for .car models.
	};

	typedef std::vector<Vertex> Vertices;

	struct Animation
	{
		unsigned int id; // animation id from original file
		unsigned int first_frame;
		unsigned int frame_count;
	};

	unsigned int frame_count;
	std::vector<Animation> animations;
	Vertices vertices;
	std::vector<unsigned short> regular_triangles_indeces;
	std::vector<unsigned short> transparent_triangles_indeces;

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

} // namespace ChasmReverse
