#pragma once

#include "../rendering_context.hpp"
#include "i_map_drawer.hpp"

namespace PanzerChasm
{

class MapDrawerSoft final : public IMapDrawer
{
public:
	MapDrawerSoft(
		Settings& settings,
		const GameResourcesConstPtr& game_resources,
		const RenderingContextSoft& rendering_context );
	virtual ~MapDrawerSoft() override;

	virtual void SetMap( const MapDataConstPtr& map_data ) override;

	virtual void Draw(
		const MapState& map_state,
		const m_Mat4& view_rotation_and_projection_matrix,
		const m_Vec3& camera_position,
		EntityId player_monster_id ) override;

	virtual void DrawWeapon(
		const WeaponState& weapon_state,
		const m_Mat4& projection_matrix,
		const m_Vec3& camera_position,
		float x_angle, float z_angle ) override;

private:
};

} // PanzerChasm
