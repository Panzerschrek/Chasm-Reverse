#pragma once

#include <vec.hpp>

namespace PanzerChasm
{

// Returns true, if collided
bool CollideCircleWithLineSegment(
	const m_Vec2& v0,
	const m_Vec2& v1,
	const m_Vec2& circle_center,
	float circle_radius,
	m_Vec2& out_pos );

bool CircleIntersectsWithSquare(
	const m_Vec2& cirecle_center,
	float circle_radius,
	unsigned int square_x, unsigned int square_y );

} // namespace PanzerChasm
