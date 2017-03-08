#pragma once
#include <matrix.hpp>

#include "map_state.hpp"
#include "weapon_state.hpp"

namespace PanzerChasm
{

class IMapDrawer
{
public:
	virtual ~IMapDrawer(){}

	virtual void SetMap( const MapDataConstPtr& map_data )= 0;

	virtual void Draw(
		const MapState& map_state,
		const m_Mat4& view_rotation_and_projection_matrix,
		const m_Vec3& camera_position,
		EntityId player_monster_id )= 0;

	virtual void DrawWeapon(
		const WeaponState& weapon_state,
		const m_Mat4& projection_matrix,
		const m_Vec3& camera_position,
		float x_angle, float z_angle )= 0;
};

} // namespace PanzerChasm
