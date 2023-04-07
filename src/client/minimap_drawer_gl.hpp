#pragma once

#include <glsl_program.hpp>
#include <polygon_buffer.hpp>

#include "../fwd.hpp"
#include "../rendering_context.hpp"
#include "fwd.hpp"
#include "i_minimap_drawer.hpp"

namespace PanzerChasm
{

class MinimapDrawerGL final : public IMinimapDrawer
{
public:
	MinimapDrawerGL(
		Settings& settings,
		const GameResourcesConstPtr& game_resources,
		const RenderingContextGL& rendering_context );
	virtual ~MinimapDrawerGL() override;

	virtual void SetMap( const MapDataConstPtr& map_data ) override;

	virtual void Draw(
		const MapState& map_state, const MinimapState& minimap_state,
		bool force_all_visible,
		const m_Vec2& camera_position,
		float scale, float view_angle ) override;

private:
	void BindColor( unsigned char color_index );
	void UpdateDynamicWalls( const MapState& map_state );
	void UpdateWallsVisibility( const MinimapState& minimap_state, bool force_all_visible );

private:
	typedef m_Vec2 WallLineVertex;

private:
	const GameResourcesConstPtr game_resources_;
	const RenderingContextGL rendering_context_;

	GLuint visibility_texture_id_= ~0u;
	GLuint dummy_visibility_texture_id_= ~0u;

	MapDataConstPtr current_map_data_;

	r_GLSLProgram lines_shader_;

	r_PolygonBuffer walls_buffer_;
	std::vector<WallLineVertex> dynamic_walls_vertices_;
	std::vector<unsigned char> visibility_texture_data_;

	unsigned int first_static_walls_vertex_= 0u;
	unsigned int first_dynamic_walls_vertex_= 0u;
	unsigned int arrow_vertices_offset_= 0u;
	unsigned int framing_vertices_offset_= 0u;
};

} // namespace PanzerChasm
