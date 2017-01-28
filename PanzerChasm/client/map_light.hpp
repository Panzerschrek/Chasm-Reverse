#pragma once

#include <framebuffer.hpp>
#include <glsl_program.hpp>
#include <polygon_buffer.hpp>
#include <texture.hpp>

#include "../fwd.hpp"
#include "../map_loader.hpp"
#include "../rendering_context.hpp"
#include "fwd.hpp"

namespace PanzerChasm
{

class MapLight final
{
public:
	MapLight(
		const GameResourcesConstPtr& game_resources,
		const RenderingContext& rendering_context );
	~MapLight();

	void SetMap( const MapDataConstPtr& map_data );

	void Update( const MapState& map_state );

	void GetStaticWallLightmapCoord( unsigned int static_wall_index, unsigned char* out_coord_xy ) const;
	void GetDynamicWallLightmapCoord( unsigned int dynamic_wall_index, unsigned char* out_coord_xy ) const;

	const r_Texture& GetFloorLightmap() const;
	const r_Texture& GetWallsLightmap() const;

private:
	struct WallVertex
	{
		short pos[2]; // 8.8 format
		unsigned char lightmap_coord_xy[2];
		char normal[2];
	};

	SIZE_ASSERT( WallVertex, 8u );

private:
	void PrepareMapWalls( const MapData& map_data );
	void UpdateLightOnDynamicWalls( const MapState& map_state );
	void DrawFloorLight( const MapData::Light& light );
	void DrawWallsLight( const MapData::Light& light );

private:
	const GameResourcesConstPtr game_resources_;

	r_Texture ambient_lightmap_texture_;

	// Floor
	r_Framebuffer base_floor_lightmap_;
	r_Framebuffer final_floor_lightmap_;

	// Walls
	r_Framebuffer base_walls_lightmap_;
	r_Framebuffer final_walls_lightmap_;

	std::vector<WallVertex> walls_vertices_;
	r_PolygonBuffer walls_vertex_buffer_;
	unsigned int static_walls_first_vertex_= 0u;
	unsigned int dynamic_walls_first_vertex_= 0u;

	// Shadowmap
	r_Framebuffer shadowmap_;

	// Shaders
	r_GLSLProgram floor_light_pass_shader_;
	r_GLSLProgram floor_ambient_light_pass_shader_;
	r_GLSLProgram walls_light_pass_shader_;
	r_GLSLProgram walls_ambient_light_pass_shader_;

	r_GLSLProgram shadowmap_shader_;

	MapDataConstPtr map_data_;

	std::vector<bool> updated_dynamic_walls_flags_;
};

} // namespace PanzerChasm
