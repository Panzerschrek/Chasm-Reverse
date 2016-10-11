#pragma once
#include <vector>

#include "vfs.hpp"

namespace ChasmReverse
{

struct Model final
{
	struct Vertex
	{
		float pos[3];
		float tex_coord[2];
	};

	std::vector<Vertex> vertices;
	std::vector<unsigned short> regular_triangles_indeces;
	std::vector<unsigned short> transparent_triangles_indeces;

	unsigned int texture_size[2];
	std::vector<unsigned char> texture_data;
};

void LoadModel( const Vfs::FileContent& model_file, Model& out_model );

} // namespace ChasmReverse
