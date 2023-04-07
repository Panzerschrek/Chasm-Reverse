#pragma once

#include "vfs.hpp"

namespace PanzerChasm
{

struct ObjSprite final
{
	unsigned int size[2];
	unsigned int frame_count;

	// all frames data
	std::vector<unsigned char> data;
};

void LoadObjSprite( const Vfs::FileContent& obj_file, ObjSprite& out_sprite );

} // namespace PanzerChasm
