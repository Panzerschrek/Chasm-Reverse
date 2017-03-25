#pragma once
#include "map_state.hpp"

namespace PanzerChasm
{

constexpr unsigned int g_wall_texture_height= 128u;
constexpr unsigned int g_max_wall_texture_width= 128u;

void SortEffectsSprites(
	const MapState::SpriteEffects& effects_sprites,
	const m_Vec3& camera_position,
	std::vector<const MapState::SpriteEffect*>& out_sorted_sprites );

} // namespace PanzerChasm
