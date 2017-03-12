#pragma once

#include "../rendering_context.hpp"
#include "i_minimap_drawer.hpp"

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
private:
};

} // namespace PanzerChasm
