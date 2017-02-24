#pragma once

#include <glsl_program.hpp>
#include <polygon_buffer.hpp>

#include "../fwd.hpp"
#include "../rendering_context.hpp"

namespace PanzerChasm
{

class MinimapDrawer final
{
public:
	MinimapDrawer(
		Settings& settings,
		const GameResourcesConstPtr& game_resources,
		const RenderingContext& rendering_context );
	~MinimapDrawer();

	void SetMap( const MapDataConstPtr& map_data );

	void Draw( const m_Vec2& camera_position, float view_angle );

private:
	typedef m_Vec2 WallLineVertex;

private:
	const RenderingContext rendering_context_;

	MapDataConstPtr current_map_data_;

	r_GLSLProgram lines_shader_;

	r_PolygonBuffer walls_buffer_;
	std::vector<WallLineVertex> dynamic_walls_vertices_;

	unsigned int first_static_walls_vertex_= 0u;
	unsigned int first_dynamic_walls_vertex_= 0u;
	unsigned int arrow_vertices_offset_= 0u;
	unsigned int framing_vertices_offset_= 0u;
};

} // namespace PanzerChasm
