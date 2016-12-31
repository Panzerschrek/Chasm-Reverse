#pragma once
#include "../game_constants.hpp"
#include "collisions.hpp"

#include "collision_index.hpp"

namespace PanzerChasm
{

template<class Func>
void CollisionIndex::ProcessElementsInRadius(
	const m_Vec2& pos, const float radius,
	const Func& func ) const
{
	const float radius_extended= radius + c_fetch_distance_eps_;

	const int x_start= std::max( static_cast<int>( std::floor( pos.x - radius_extended ) ), 0 );
	const int x_end  = std::min( static_cast<int>( std::floor( pos.x + radius_extended ) ), int(MapData::c_map_size - 1u) );
	const int y_start= std::max( static_cast<int>( std::floor( pos.y - radius_extended ) ), 0 );
	const int y_end  = std::min( static_cast<int>( std::floor( pos.y + radius_extended ) ), int(MapData::c_map_size - 1u) );

	for( int y= y_start; y <= y_end; y++ )
	for( int x= x_start; x <= x_end; x++ )
	{
		unsigned short i= index_field_[ x + y * int(MapData::c_map_size) ];
		while( i != IndexElement::c_dummy_next )
		{
			PC_ASSERT( i <= index_elements_.size() );
			const IndexElement& element= index_elements_[i];

			func( element.index_element );
			i= element.next;
		}
	}

	// Process dynamic models without any optimizations
	for( const unsigned short dynamic_model_index : dynamic_models_indeces_ )
	{
		MapData::IndexElement element;
		element.type= MapData::IndexElement::StaticModel;
		element.index= dynamic_model_index;
		func( element );
	}
}

template<class Func>
void CollisionIndex::RayCast(
	const m_Vec3& pos, const m_Vec3& dir_normalized,
	const Func& func,
	const float max_cast_distance ) const
{
	unsigned int walls_checked= 0u;

	float end_distance_xy=
		std::min(
			max_cast_distance * dir_normalized.xy().Length(),
			float( MapData::c_map_size * 2u ) );

	if( pos.z >= 0.0f && pos.z <= GameConstants::walls_height )
	{
		m_Vec3 end_pos;

		if( dir_normalized.z > 0.0f )
		{
			if( RayIntersectXYPlane(
					GameConstants::walls_height,
					pos, dir_normalized,
					end_pos ) )
				end_distance_xy= std::min( ( end_pos - pos ).xy().Length(), end_distance_xy );
		}
		else
		{
			if( RayIntersectXYPlane(
					0.0f,
					pos, dir_normalized,
					end_pos ) )
				end_distance_xy= std::min( ( end_pos - pos ).xy().Length(), end_distance_xy );
		}
	}
	else
	{
		// TODO
	}

	m_Vec2 dir_xy= dir_normalized.xy();
	dir_xy.Normalize();

	int prev_x= std::numeric_limits<int>::max();
	int prev_y= std::numeric_limits<int>::max();

	const unsigned int max_distance_xy_i= static_cast<unsigned int>( std::ceil( end_distance_xy ) );
	for( unsigned int i= 0u; i < max_distance_xy_i; i++ )
	{
		const m_Vec2 sample_pos= pos.xy() + dir_xy * float(i);
		const int x= static_cast<int>( std::floor( sample_pos.x ) );
		const int y= static_cast<int>( std::floor( sample_pos.y ) );

		if( x == prev_x && y == prev_y )
		{
			// Nothing to do - already checked.
		}
		else if( x != prev_x && y != prev_y && i != 0u )
		{
			// Check this and neighbor cells.
			const int cells[6]= { x, y,   prev_x, y,  x, prev_y };

			for( unsigned int c= 0u; c < 6u; c+= 2u )
			{
				if( cells[c+0u] >= 0 && cells[c+0u] < int(MapData::c_map_size) &&
					cells[c+1u] >= 0 && cells[c+1u] < int(MapData::c_map_size) )
				{
					unsigned short index= index_field_[ cells[c] + cells[c+1u] * int(MapData::c_map_size) ];
					while( index != IndexElement::c_dummy_next )
					{
						PC_ASSERT( index <= index_elements_.size() );
						const IndexElement& element= index_elements_[index];

						if( func( element.index_element ) )
							return;

						index= element.next;

						walls_checked++;
					}
				}
			}
		}
		else
		{
			// Check one cell.
			if( x >= 0 && x < int(MapData::c_map_size) &&
				y >= 0 && y < int(MapData::c_map_size) )
			{
				unsigned short index= index_field_[ x + y * int(MapData::c_map_size) ];
				while( index != IndexElement::c_dummy_next )
				{
					PC_ASSERT( index <= index_elements_.size() );
					const IndexElement& element= index_elements_[index];

					if( func( element.index_element ) )
						return;

					index= element.next;

					walls_checked++;
				}
			}
		}

		prev_x= x;
		prev_y= y;
	} // line trace

	// Process dynamic models without any optimizations.
	for( const unsigned short dynamic_model_index : dynamic_models_indeces_ )
	{
		MapData::IndexElement element;
		element.type= MapData::IndexElement::StaticModel;
		element.index= dynamic_model_index;
		func( element );
	}
}

} // namespace PanzerChasm
