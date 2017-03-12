#include "../assert.hpp"

#include "minimap_drawer_soft.hpp"

namespace PanzerChasm
{

MinimapDrawerSoft::MinimapDrawerSoft(
	Settings& settings,
	const GameResourcesConstPtr& game_resources,
	const RenderingContextSoft& rendering_context )
{
	// TODO
	PC_UNUSED( settings );
	PC_UNUSED( game_resources );
	PC_UNUSED( rendering_context );
}

MinimapDrawerSoft::~MinimapDrawerSoft()
{}

void MinimapDrawerSoft::SetMap( const MapDataConstPtr& map_data )
{
	// TODO
	PC_UNUSED( map_data );
}

void MinimapDrawerSoft::Draw(
	const MapState& map_state, const MinimapState& minimap_state,
	const m_Vec2& camera_position, const float view_angle )
{
	// TODO
	PC_UNUSED( map_state );
	PC_UNUSED( minimap_state );
	PC_UNUSED( camera_position );
	PC_UNUSED( view_angle );
}

} // namespace PanzerChasm
