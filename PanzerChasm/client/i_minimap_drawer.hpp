#pragma once
#include "map_state.hpp"
#include "minimap_state.hpp"

namespace PanzerChasm
{

class IMinimapDrawer
{
public:
	virtual ~IMinimapDrawer(){}

	virtual void SetMap( const MapDataConstPtr& map_data )= 0;

	virtual void Draw(
		const MapState& map_state, const MinimapState& minimap_state,
		bool force_all_visible,
		const m_Vec2& camera_position,
		float scale, float view_angle )= 0;
};

} // namespace PanzerChasm
