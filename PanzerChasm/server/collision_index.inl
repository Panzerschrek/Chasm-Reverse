#pragma once
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

} // namespace PanzerChasm
