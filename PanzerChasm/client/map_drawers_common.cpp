#include <algorithm>

#include "../map_loader.hpp"
#include "../math_utils.hpp"

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

unsigned int GetModelBMPSpritePhase( const MapState::StaticModel& model )
{
	// Generate pseudo-random animation phase for sprite, because synchronous animation of nearby sprites looks ugly.
	return static_cast<unsigned int>( model.pos.x * 13.0f + model.pos.y * 19.0f + model.angle * 29.0f );
}

bool GetNearestLightSourcePos(
	const m_Vec3& pos,
	const MapData& map_data,
	const MapState& map_state,
	const bool use_dynamic_lights,
	m_Vec3& out_light_pos )
{
	float nearest_source_square_distance= Constants::max_float;
	m_Vec3 nearest_source( 0.0f, 0.0f, 0.0f );

	for( const MapData::Light& light : map_data.lights )
	{
		const float square_distance= ( light.pos - pos.xy() ).SquareLength();
		if( square_distance < nearest_source_square_distance )
		{
			nearest_source.x= light.pos.x;
			nearest_source.y= light.pos.y;
			nearest_source_square_distance= square_distance;
		}
	}

	if( use_dynamic_lights )
	{
		for( const MapState::LightFlash& light_flash : map_state.GetLightFlashes() )
		{
			const float square_distance= ( light_flash.pos - pos.xy() ).SquareLength();
			if( square_distance < nearest_source_square_distance )
			{
				nearest_source.x= light_flash.pos.x;
				nearest_source.y= light_flash.pos.y;
				nearest_source_square_distance= square_distance;
			}
		}

		for( const MapState::LightSourcesContainer::value_type& light_source_value : map_state.GetLightSources() )
		{
			const MapState::LightSource& light= light_source_value.second;

			const float square_distance= ( light.pos - pos.xy() ).SquareLength();
			if( square_distance < nearest_source_square_distance )
			{
				nearest_source.x= light.pos.x;
				nearest_source.y= light.pos.y;
				nearest_source_square_distance= square_distance;
			}
		}
	}

	if( nearest_source_square_distance < 4.0f * 4.0f )
	{
		nearest_source.z= 4.0f;
		out_light_pos= nearest_source;
		return true;
	}

	return false;
}

} // namespace PanzerChasm
