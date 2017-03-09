#include "../assert.hpp"

#include "map_drawer_soft.hpp"

namespace PanzerChasm
{

MapDrawerSoft::MapDrawerSoft(
	Settings& settings,
	const GameResourcesConstPtr& game_resources,
	const RenderingContextSoft& rendering_context )
{
	// TODO
	PC_UNUSED( settings );
	PC_UNUSED( game_resources );
	PC_UNUSED( rendering_context );
}

MapDrawerSoft::~MapDrawerSoft()
{}

void MapDrawerSoft::SetMap( const MapDataConstPtr& map_data )
{
	// TODO
	PC_UNUSED( map_data );
}

void MapDrawerSoft::Draw(
	const MapState& map_state,
	const m_Mat4& view_rotation_and_projection_matrix,
	const m_Vec3& camera_position,
	const EntityId player_monster_id )
{
	// TODO
	PC_UNUSED( map_state );
	PC_UNUSED( view_rotation_and_projection_matrix );
	PC_UNUSED( camera_position );
	PC_UNUSED( player_monster_id );
}

void MapDrawerSoft::DrawWeapon(
	const WeaponState& weapon_state,
	const m_Mat4& projection_matrix,
	const m_Vec3& camera_position,
	const float x_angle, const float z_angle )
{
	// TODO
	PC_UNUSED( weapon_state );
	PC_UNUSED( projection_matrix );
	PC_UNUSED( camera_position );
	PC_UNUSED( x_angle );
	PC_UNUSED( z_angle );
}

} // PanzerChasm
