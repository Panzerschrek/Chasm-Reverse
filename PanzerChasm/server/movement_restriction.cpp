#include "../assert.hpp"

#include "movement_restriction.hpp"

namespace PanzerChasm
{

MovementRestriction::MovementRestriction()
{}

MovementRestriction::~MovementRestriction()
{}

void MovementRestriction::AddRestriction( const m_Vec2& normal )
{
	if( planes_count_ >= c_max_restriction_planes )
		return;

	normals_[ planes_count_ ]= normal;
	planes_count_++;
}

bool MovementRestriction::MovementIsBlocked( const m_Vec2& movement_direction_normalized ) const
{
	for( unsigned int i= 0u; i < planes_count_; i++ )
		if( normals_[i] * movement_direction_normalized < -0.1f )
			return true;

	return false;
}

} // namespace PanzerChasm
