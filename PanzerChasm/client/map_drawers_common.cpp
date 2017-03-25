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

bool BBoxIsOutsideView(
	const ViewClipPlanes& clip_planes,
	const m_BBox3& bbox,
	const m_Mat4& bbox_mat )
{
	for( const m_Plane3& plane : clip_planes )
	{
		bool is_inside= false;
		for( unsigned int z= 0u; z < 2u; z++ )
		for( unsigned int y= 0u; y < 2u; y++ )
		for( unsigned int x= 0u; x < 2u; x++ )
		{
			const m_Vec3 point(
				x == 0 ? bbox.min.x : bbox.max.x,
				y == 0 ? bbox.min.y : bbox.max.y,
				z == 0 ? bbox.min.z : bbox.max.z );

			if( plane.IsPointAheadPlane( point * bbox_mat ) )
			{
				is_inside= true;
				goto box_vertices_check_end;
			}
		}
		box_vertices_check_end:

		if( !is_inside ) // bbox is behind of one of clip planes
			return true;
	}

	return false;
}

} // namespace PanzerChasm
