#include <algorithm>

#include <matrix.hpp>

#include "../assert.hpp"
#include "../math_utils.hpp"

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

bool CollideCircleWithSquare(
	const m_Vec2& square_center,
	const float angle,
	const float square_side_half,
	const m_Vec2& circle_center,
	const float circle_radius,
	m_Vec2& out_pos )
{
	if( square_center == circle_center )
	{
		out_pos= square_center;
		return true;
	}

	const m_Vec2 vec_to_circle= circle_center - square_center;

	m_Mat3 rot_mat;
	rot_mat.RotateZ( -angle );

	const m_Vec2 vec_to_circle_transformed= vec_to_circle * rot_mat;
	m_Vec2 out_vec_transformed;

	const float min_distance= square_side_half + circle_radius;

	if( std::abs( vec_to_circle_transformed.x ) > std::abs( vec_to_circle_transformed.y ) )
	{
		const float abs_x= std::abs(vec_to_circle_transformed.x);
		if( abs_x < min_distance )
			out_vec_transformed= vec_to_circle_transformed * min_distance / abs_x;
		else
			return false;
	}
	else
	{
		const float abs_y= std::abs(vec_to_circle_transformed.y);
		if( abs_y < min_distance )
			out_vec_transformed= vec_to_circle_transformed * min_distance / abs_y;
		else
			return false;
	}

	// TODO - sometimes, we must push position from corners, but not sides.
	// But, it seems, that original game behaviour is same.

	rot_mat.Transpose();
	out_pos= square_center + ( out_vec_transformed * rot_mat );
	return true;
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

bool RayIntersectXYPlane(
	const float plane_z,
	const m_Vec3& ray_start_point,
	const m_Vec3& ray_direction_normalized,
	m_Vec3& out_pos )
{
	if( ray_direction_normalized.z == 0.0f )
		return false;

	const float signed_distance_to_plane= plane_z - ray_start_point.z;
	const float distance_to_intersection_point= signed_distance_to_plane / ray_direction_normalized.z;

	const m_Vec3 vec_to_intersection_point= ray_direction_normalized * distance_to_intersection_point;
	if( vec_to_intersection_point * ray_direction_normalized <= 0.0f )
		return false;

	out_pos= ray_start_point + vec_to_intersection_point;
	return true;
}

bool RayIntersectCylinder(
	const m_Vec2& cylinder_center, const float cylinder_radius,
	const float cylinder_bottom, const float cylinder_top,
	const m_Vec3& ray_start_point,
	const m_Vec3& ray_direction_normalized,
	m_Vec3& out_pos )

{
	float min_square_distance_to_candidate= Constants::max_float;

	const float square_cylinder_radius= cylinder_radius * cylinder_radius;

	const m_Vec2 ray_direction_xy= ray_direction_normalized.xy();
	const float ray_direction_xy_square_length= ray_direction_xy.SquareLength();

	if( ray_direction_xy_square_length != 0.0f )
	{
		// 2D plase start
		const float ray_direction_xy_length= std::sqrt( ray_direction_xy_square_length );
		const m_Vec2 ray_direction_xy_normalized= ray_direction_xy / ray_direction_xy_length;

		const m_Vec2 vec_to_cylinder_center= cylinder_center - ray_start_point.xy();
		const float vec_to_cylinder_center_projected_to_ray_length= vec_to_cylinder_center * ray_direction_xy_normalized;
		const m_Vec2 vec_to_cylinder_center_projected_to_ray=
			vec_to_cylinder_center_projected_to_ray_length * ray_direction_xy_normalized;

		const m_Vec2 vec_from_cylinder_center_to_ray_line= vec_to_cylinder_center - vec_to_cylinder_center_projected_to_ray;
		const float square_distance_from_cylinder_center_to_line= vec_from_cylinder_center_to_ray_line.SquareLength();

		const float square_distance_between_intersection_point_and_projection_point=
			square_cylinder_radius - square_distance_from_cylinder_center_to_line;
		if( square_distance_between_intersection_point_and_projection_point <= 0.0f )
			goto check_sides;

		const float distance_between_intersection_point_and_projection_point=
			std::sqrt( square_distance_between_intersection_point_and_projection_point );

		const float signed_distanse_to_intersection_point= vec_to_cylinder_center_projected_to_ray_length - distance_between_intersection_point_and_projection_point;
		if( signed_distanse_to_intersection_point < 0.0f )
			goto check_sides;

		// 2D phase end
		const float distanse_to_intersection_point_3d= signed_distanse_to_intersection_point / ray_direction_xy_length;

		const m_Vec3 intersection_point=
			ray_start_point +
			ray_direction_normalized * distanse_to_intersection_point_3d;

		if( intersection_point.z >= cylinder_bottom &&
			intersection_point.z <= cylinder_top )
		{
			min_square_distance_to_candidate= distanse_to_intersection_point_3d * distanse_to_intersection_point_3d;
			out_pos= intersection_point;
		}
	}

	check_sides:

	// Check cylinder sides
	for( unsigned int i= 0u; i < 2u; i++ )
	{
		m_Vec3 candidate_pos;
		if( RayIntersectXYPlane(
				i == 0u ? cylinder_top : cylinder_bottom,
				ray_start_point,
				ray_direction_normalized,
				candidate_pos ) )
		{
			const m_Vec2 vec_from_cylinder_center_to_candidate= candidate_pos.xy() - cylinder_center;
			if( vec_from_cylinder_center_to_candidate.SquareLength() > square_cylinder_radius )
				continue;

			const m_Vec3 vec_to_candidate= candidate_pos - ray_start_point;
			const float square_distance= vec_to_candidate.SquareLength();
			if( square_distance < min_square_distance_to_candidate )
			{
				min_square_distance_to_candidate= square_distance;
				out_pos= candidate_pos;
			}
		}
	}

	return min_square_distance_to_candidate < Constants::max_float;
}

float DistanceToCylinder(
	const m_Vec2& cylinder_center, const float cylinder_radius,
	const float cylinder_bottom, const float cylinder_top,
	const m_Vec3& pos )
{
	const float distance_to_cylinder_edge_xy= ( cylinder_center - pos.xy() ).Length() - cylinder_radius;

	if( pos.z >= cylinder_top )
	{
		if( distance_to_cylinder_edge_xy <= 0.0f )
			return pos.z - cylinder_top;

		const m_Vec2 vec_to_circle( distance_to_cylinder_edge_xy, pos.z - cylinder_top );
		return vec_to_circle.Length();
	}
	else if( pos.z <= cylinder_bottom )
	{
		if( distance_to_cylinder_edge_xy <= 0.0f )
			return cylinder_bottom - pos.z;

		const m_Vec2 vec_to_circle( distance_to_cylinder_edge_xy, cylinder_bottom - pos.z );
		return vec_to_circle.Length();
	}
	else
	{
		return std::max( distance_to_cylinder_edge_xy, 0.0f );
	}
}

} // namespace PanzerChasm
