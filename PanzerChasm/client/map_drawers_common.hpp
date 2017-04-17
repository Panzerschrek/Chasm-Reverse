#pragma once
#include <bbox.hpp>

#include "map_state.hpp"
#include "i_map_drawer.hpp"

namespace PanzerChasm
{

constexpr unsigned int g_wall_texture_height= 128u;
constexpr unsigned int g_max_wall_texture_width= 128u;

// TODO - maybe this points differnet for differnet weapons?
// Crossbow: m_Vec3( 0.2f, 0.7f, -0.45f )
const m_Vec3 g_weapon_shift= m_Vec3( 0.2f, 0.7f, -0.45f );
const m_Vec3 g_weapon_change_shift= m_Vec3( 0.0f, -0.9f, 0.0f );

void SortEffectsSprites(
	const MapState::SpriteEffects& effects_sprites,
	const m_Vec3& camera_position,
	std::vector<const MapState::SpriteEffect*>& out_sorted_sprites );

bool BBoxIsOutsideView(
	const ViewClipPlanes& clip_planes,
	const m_BBox3& bbox,
	const m_Mat4& bbox_mat );

} // namespace PanzerChasm
