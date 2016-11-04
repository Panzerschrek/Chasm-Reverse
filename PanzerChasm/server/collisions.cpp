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

bool CircleIntersectsWithSquare(
	const m_Vec2& cirecle_center,
	float circle_radius,
	unsigned int square_x, unsigned int square_y )
{
	const float c_square_inner_radius= 0.5f;
	const float c_square_outer_radius= 0.5f * std::sqrt( 2.0f );

	const m_Vec2 square_start{ float(square_x), float(square_y) };
	const m_Vec2 square_center= square_start + m_Vec2( 0.5f, 0.5f );

	const m_Vec2 vec_from_square_center_to_circle_center= cirecle_center - square_center;
	const float square_distance_to_circle= vec_from_square_center_to_circle_center.SquareLength();

	const float reject_distance= c_square_outer_radius + circle_radius;
	if( square_distance_to_circle > reject_distance * reject_distance )
		return false;

	const float accept_distance= c_square_inner_radius + circle_radius;
	if( square_distance_to_circle < accept_distance * accept_distance )
		return true;

	m_Vec2 tmp;
	if( CollideCircleWithLineSegment(
			square_start, square_start + m_Vec2( 1.0f, 0.0f ),
			cirecle_center, circle_radius,
			tmp ) )
		return true;

	if( CollideCircleWithLineSegment(
			square_start + m_Vec2( 1.0f, 0.0f ), square_start + m_Vec2( 1.0f, 1.0f ),
			cirecle_center, circle_radius,
			tmp ) )
		return true;

	if( CollideCircleWithLineSegment(
			square_start + m_Vec2( 1.0f, 1.0f ), square_start + m_Vec2( 0.0f, 1.0f ),
			cirecle_center, circle_radius,
			tmp ) )
		return true;

	if( CollideCircleWithLineSegment(
			square_start + m_Vec2( 0.0f, 1.0f ), square_start,
			cirecle_center, circle_radius,
			tmp ) )
		return true;

	return false;
}

bool RayIntersectWall(
	const m_Vec2& v0, const m_Vec2& v1,
	float z_bottom, float z_top,
	const m_Vec3& ray_start_point,
	const m_Vec3& ray_direction_normalized,
	m_Vec3& out_pos )
{
	const m_Vec2 plane_vector( v1 - v0 );
	const m_Vec3 plane_normal= m_Vec3( plane_vector.y, -plane_vector.x, 0.0f ) / plane_vector.Length();

	float normal_dir_dot= ray_direction_normalized * plane_normal;
	if( normal_dir_dot == 0.0f )
		return false;

	const m_Vec3 vec_to_plane= m_Vec3( v0, z_bottom ) - ray_start_point;
	const float signed_distance_to_plane= vec_to_plane * plane_normal;

	const m_Vec3 dir_vec_to_plane=
		ray_direction_normalized * ( signed_distance_to_plane / normal_dir_dot );

	if( dir_vec_to_plane * ray_direction_normalized <= 0.0f )
		return false;

	const m_Vec3 intersection_point= ray_start_point + dir_vec_to_plane;

	if( intersection_point.z < z_bottom || intersection_point.z > z_top )
		return false;

	const m_Vec2 vec_from_intersection_point_to_wall_point[2]=
	{
		v0 - intersection_point.xy(),
		v1 - intersection_point.xy(),
	};

	if( vec_from_intersection_point_to_wall_point[0] * vec_from_intersection_point_to_wall_point[1]
		> 0.0f )
		return false;

	out_pos= intersection_point;
	return true;
}

} // namespace PanzerChasm
