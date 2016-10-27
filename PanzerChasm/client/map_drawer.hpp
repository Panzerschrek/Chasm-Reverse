#pragma once

#include <glsl_program.hpp>
#include <polygon_buffer.hpp>
#include <texture.hpp>

#include "../game_resources.hpp"
#include "../map_loader.hpp"
#include "../rendering_context.hpp"

namespace PanzerChasm
{

class MapDrawer final
{
public:
	MapDrawer(
		GameResourcesConstPtr game_resources,
		const RenderingContext& rendering_context );
	~MapDrawer();

	void SetMap( const MapDataConstPtr& map_data );

	void Draw( const m_Mat4& view_matrix );

private:
	struct FloorGeometryInfo
		{
			unsigned int first_vertex_number;
			unsigned int vertex_count;
		};

private:
	void LoadFloorsTextures( const MapData& map_data );
	void LoadWallsTextures( const MapData& map_data );

	void LoadFloors( const MapData& map_data );
	void LoadWalls( const MapData& map_data );

private:
	const GameResourcesConstPtr game_resources_;
	const RenderingContext rendering_context_;

	MapDataConstPtr current_map_data_;

	r_Texture lightmap_;

	GLuint floor_textures_array_id_= ~0;
	GLuint wall_textures_array_id_= ~0;

	r_GLSLProgram floors_shader_;
	r_PolygonBuffer floors_geometry_;
	// 0 - ceiling, 1 - floor
	FloorGeometryInfo floors_geometry_info[2];

	r_GLSLProgram walls_shader_;
	r_PolygonBuffer walls_geometry_;
	r_PolygonBuffer dynamic_walls_geometry_;
};

} // PanzerChasm
