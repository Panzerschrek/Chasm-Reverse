#include "../assert.hpp"

#include "movement_restriction.hpp"

namespace PanzerChasm
{

MovementRestriction::MovementRestriction()
{}

MovementRestriction::~MovementRestriction()
{}

void MovementRestriction::AddRestriction( const m_Vec2& normal, const MapData::IndexElement& map_element )
{
	m_Vec2 normal_normalized= normal;
	normal_normalized.Normalize();

	if( planes_count_ < 2 )
	{
		// Just add new plane.
		restriction_planes_[ planes_count_ ].normal= normal_normalized;
		restriction_planes_[ planes_count_ ].map_element= map_element;
		planes_count_++;

		return;
	}

	PC_ASSERT( planes_count_ == 2u );

	const float current_planes_dot= restriction_planes_[0].normal * restriction_planes_[1].normal;

	// Make angle between two restriction planes sharper.
	if( normal * restriction_planes_[0].normal < current_planes_dot )
	{
		restriction_planes_[1].normal= normal_normalized;
		restriction_planes_[1].map_element= map_element;
	}
	else if( normal * restriction_planes_[1].normal < current_planes_dot )
	{
		restriction_planes_[0].normal= normal_normalized;
		restriction_planes_[0].map_element= map_element;
	}
}

} // namespace PanzerChasm
