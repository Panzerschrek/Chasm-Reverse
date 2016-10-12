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

	struct ModelGeometry
	{
		unsigned int first_index;
		unsigned int index_count;
		unsigned int first_transparent_index;
		unsigned int transparent_index_count;
	};

	struct LevelModel
	{
		m_Vec3 pos;
		float angle;
		unsigned char id;
	};

	typedef std::vector<LevelModel> LevelModels;

private:
	void LoadFloorsTextures(
		const Vfs::FileContent& floors_file,
		const unsigned char* palette );

	void LoadWallsTextures(
		const Vfs& vfs,
		const Vfs::FileContent& resources_file,
		const unsigned char* palette,
		bool* out_textures_exist_flags );

	void LoadModels(
		const Vfs& vfs,
		const Vfs::FileContent& resources_file,
		const unsigned char* palette );

private:
	r_Texture lightmap_;

	GLuint floor_textures_array_id_= ~0;
	GLuint wall_textures_array_id_= ~0;
	GLuint models_textures_array_id_= ~0;

	r_GLSLProgram floors_shader_;
	r_PolygonBuffer floors_geometry_;

	r_GLSLProgram walls_shader_;
	r_PolygonBuffer walls_geometry_;

	// 0 - ceiling, 1 - floor
	FloorGeometryInfo floors_geometry_info[2];

	r_GLSLProgram models_shader_;
	std::vector<ModelGeometry> models_geometry_;
	r_PolygonBuffer models_geometry_data_;

	LevelModels level_models_;
};

} // namespace ChasmReverse
