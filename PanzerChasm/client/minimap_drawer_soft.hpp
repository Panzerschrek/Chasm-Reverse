#pragma once

#include "../rendering_context.hpp"
#include "i_minimap_drawer.hpp"
#include "software_renderer/fixed.hpp"

namespace PanzerChasm
{

class MinimapDrawerSoft final : public IMinimapDrawer
{
public:
	MinimapDrawerSoft(
		Settings& settings,
		const GameResourcesConstPtr& game_resources,
		const RenderingContextSoft& rendering_context );
	virtual ~MinimapDrawerSoft() override;

	virtual void SetMap( const MapDataConstPtr& map_data ) override;

	virtual void Draw(
		const MapState& map_state, const MinimapState& minimap_state,
		const m_Vec2& camera_position, float view_angle ) override;

	void DrawLine(
		fixed16_t x0, fixed16_t y0,
		fixed16_t x1, fixed16_t y1 );

private:
	const RenderingContextSoft rendering_context_;
	const GameResourcesConstPtr game_resources_;

	MapDataConstPtr current_map_data_;

	int x_min_, y_min_, x_max_, y_max_;
	fixed16_t x_min_f_, y_min_f_, x_max_f_, y_max_f_;

	uint32_t line_color_;
	int line_width_;
};

} // namespace PanzerChasm
