#pragma once
#include <memory>

#include <glsl_program.hpp>
#include <matrix.hpp>
#include <panzer_ogl_lib.hpp>
#include <polygon_buffer.hpp>
#include <texture.hpp>

#include "vfs.hpp"

namespace ChasmReverse
{

class MapViewer final
{
public:
	MapViewer( const std::shared_ptr<Vfs>& vfs, unsigned int map_number );
	~MapViewer();

	void Draw( const m_Mat4& view_matrix );

private:
	struct FloorGeometryInfo
	{
		unsigned int first_vertex_number;
		unsigned int vertex_count;
	};

private:
	r_Texture lightmap_;

	GLuint floor_textures_array_id_= ~0;

	r_GLSLProgram floors_shader_;
	r_PolygonBuffer floors_geometry_;

	// 0 - ceiling, 1 - floor
	FloorGeometryInfo floors_geometry_info[2];
};

} // namespace ChasmReverse
