#pragma once

#include <vec.hpp>

namespace PanzerChasm
{

class MovementRestriction final
{
public:
	MovementRestriction();
	~MovementRestriction();

	void AddRestriction( const m_Vec2& normal );

	bool MovementIsBlocked( const m_Vec2& movement_direction_normalized ) const;

private:
	static constexpr unsigned int c_max_restriction_planes= 8u;

	m_Vec2 normals_[ c_max_restriction_planes ];
	unsigned int planes_count_= 0u;
};

} // namespace PanzerChasm
