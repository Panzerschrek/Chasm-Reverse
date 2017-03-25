#pragma once
#include <bbox.hpp>

#include "map_state.hpp"
#include "i_map_drawer.hpp"

namespace PanzerChasm
{

constexpr unsigned int g_wall_texture_height= 128u;
constexpr unsigned int g_max_wall_texture_width= 128u;

void SortEffectsSprites(
	const MapState::SpriteEffects& effects_sprites,
	const m_Vec3& camera_position,
	std::vector<const MapState::SpriteEffect*>& out_sorted_sprites );

bool BBoxIsOutsideView(
	const ViewClipPlanes& clip_planes,
	const m_BBox3& bbox,
	const m_Mat4& bbox_mat );

} // namespace PanzerChasm
