#pragma once

#include <glsl_program.hpp>
#include <polygon_buffer.hpp>
#include <texture.hpp>

#include "../game_resources.hpp"
#include "../map_loader.hpp"
#include "../rendering_context.hpp"
#include "map_state.hpp"

namespace PanzerChasm
{

struct WallVertex;

class MapDrawer final
{
public:
	MapDrawer(
		GameResourcesConstPtr game_resources,
		const RenderingContext& rendering_context );
	~MapDrawer();

	void SetMap( const MapDataConstPtr& map_data );

	void Draw(
		const MapState& map_state,
		const m_Mat4& view_matrix );

private:
	struct FloorGeometryInfo
	{
		unsigned int first_vertex_number;
		unsigned int vertex_count;
	};

	struct ModelGeometry
	{
		unsigned int frame_count;
		unsigned int vertex_count;
		unsigned int first_vertex_index;
		unsigned int first_index;
		unsigned int index_count;
		unsigned int first_transparent_index;
		unsigned int transparent_index_count;
	};

private:
	void LoadSprites();

	void LoadFloorsTextures( const MapData& map_data );
	void LoadWallsTextures( const MapData& map_data );

	void LoadFloors( const MapData& map_data );
	void LoadWalls( const MapData& map_data );

	void LoadModels(
		const std::vector<Model>& models,
		std::vector<ModelGeometry>& out_geometry,
		r_PolygonBuffer& out_geometry_data,
		GLuint& out_textures_array ) const;

	void UpdateDynamicWalls( const MapState::DynamicWalls& dynamic_walls );

private:
	const GameResourcesConstPtr game_resources_;
	const RenderingContext rendering_context_;

	MapDataConstPtr current_map_data_;

	r_Texture lightmap_;

	GLuint floor_textures_array_id_= ~0;
	GLuint wall_textures_array_id_= ~0;
	GLuint models_textures_array_id_= ~0;
	GLuint items_textures_array_id_= ~0;

	std::vector<GLuint> sprites_textures_arrays_;

	r_GLSLProgram floors_shader_;
	r_PolygonBuffer floors_geometry_;
	// 0 - ceiling, 1 - floor
	FloorGeometryInfo floors_geometry_info[2];

	r_GLSLProgram walls_shader_;
	r_PolygonBuffer walls_geometry_;

	r_PolygonBuffer dynamic_walls_geometry_;
	std::vector<WallVertex> dynamc_walls_vertices_;

	r_GLSLProgram models_shader_;

	std::vector<ModelGeometry> models_geometry_;
	r_PolygonBuffer models_geometry_data_;

	std::vector<ModelGeometry> items_geometry_;
	r_PolygonBuffer items_geometry_data_;
};

} // PanzerChasm
