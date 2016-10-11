#pragma once
#include <memory>

#include <panzer_ogl_lib.hpp>

#include "vfs.hpp"

namespace ChasmReverse
{

class MapViewer final
{
public:
	MapViewer( const std::shared_ptr<Vfs>& vfs, unsigned int map_number );
	~MapViewer();

private:
	GLuint floor_textures_array_id_= ~0;
};

} // namespace ChasmReverse
