#pragma once
#include <vector>

#include "vfs.hpp"

namespace ChasmReverse
{

struct Submodel
{
	struct Vertex
	{
		float pos[3];
		float tex_coord[2];
		unsigned char texture_id;
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
};

struct Model final : public Submodel
{
	unsigned int texture_size[2];
	std::vector<unsigned char> texture_data;

	std::vector<Submodel> submodels;
};

void LoadModel_o3( const Vfs::FileContent& model_file, const Vfs::FileContent& animation_file, Model& out_model );
void LoadModel_car( const Vfs::FileContent& model_file, Model& out_model );

} // namespace ChasmReverse
