#include <algorithm>

#include "map_drawers_common.hpp"

namespace PanzerChasm
{

void SortEffectsSprites(
	const MapState::SpriteEffects& effects_sprites,
	const m_Vec3& camera_position,
	std::vector<const MapState::SpriteEffect*>& out_sorted_sprites )
{
	// Sort from far to near
	out_sorted_sprites.resize( effects_sprites.size() );
	for( unsigned int i= 0u; i < effects_sprites.size(); i++ )
		out_sorted_sprites[i]= & effects_sprites[i];

	std::sort(
		out_sorted_sprites.begin(),
		out_sorted_sprites.end(),
		[&]( const MapState::SpriteEffect* const a, const MapState::SpriteEffect* const b )
		{
			const m_Vec3 dir_a= camera_position - a->pos;
			const m_Vec3 dir_b= camera_position - b->pos;
			return dir_a.SquareLength() >= dir_b.SquareLength();
		});
}

} // namespace PanzerChasm
