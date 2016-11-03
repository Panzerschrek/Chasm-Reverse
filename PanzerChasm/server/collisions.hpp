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

} // namespace PanzerChasm
