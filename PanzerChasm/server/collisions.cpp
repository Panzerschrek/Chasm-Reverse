#include "../assert.hpp"

#include "collisions.hpp"

namespace PanzerChasm
{

// Returns true, if collided
bool CollideCircleWithLineSegment(
	const m_Vec2& v0,
	const m_Vec2& v1,
	const m_Vec2& circle_center,
	float circle_radius,
	m_Vec2& out_pos )
{
	const m_Vec2 v0_v1_vec= v1 - v0;
	const float v0_v1_vec_square_length= v0_v1_vec.SquareLength();

	const m_Vec2 vec_from_v0_to_circle_center= circle_center - v0;
	const float vecs_dot= vec_from_v0_to_circle_center * v0_v1_vec;

	if( vecs_dot <= 0.0f )
	{
		const float distance_to_v0= vec_from_v0_to_circle_center.Length();

		if( distance_to_v0 == 0.0f )
		{
			out_pos= v0 - v0_v1_vec * circle_radius / std::sqrt( v0_v1_vec_square_length );
			return true;
		}
		if( distance_to_v0 < circle_radius )
		{
			out_pos= v0 + vec_from_v0_to_circle_center * circle_radius / distance_to_v0;
			return true;
		}
	}
	else if( vecs_dot >= v0_v1_vec_square_length )
	{
		const m_Vec2 vec_from_v1_to_circle_center= circle_center - v1;

		const float distance_to_v1= vec_from_v1_to_circle_center.Length();

		if( distance_to_v1 == 0.0f )
		{
			out_pos= v1 + v0_v1_vec * circle_radius / std::sqrt( v0_v1_vec_square_length );
			return true;
		}
		if( distance_to_v1 < circle_radius )
		{
			out_pos= v1 + vec_from_v1_to_circle_center * circle_radius / distance_to_v1;
			return true;
		}
	}
	else
	{
		const m_Vec2 projection_to_line= v0_v1_vec * ( vecs_dot / v0_v1_vec_square_length );
		const m_Vec2 vec_from_line= vec_from_v0_to_circle_center - projection_to_line;
		const float vec_from_line_square_length= vec_from_line.SquareLength();

		if( vec_from_line_square_length == 0.0f )
		{
			// Move along normal
			const m_Vec2 normal= m_Vec2( -v0_v1_vec.y, v0_v1_vec.x ) / std::sqrt( v0_v1_vec_square_length );
			out_pos= v0 + projection_to_line + normal * circle_radius;
			return true;
		}
		else if( vec_from_line_square_length < circle_radius * circle_radius )
		{
			out_pos= v0 + projection_to_line + vec_from_line * ( circle_radius / std::sqrt( vec_from_line_square_length ) );
			return true;
		}
	}

	return false;
}

} // namespace PanzerChasm
