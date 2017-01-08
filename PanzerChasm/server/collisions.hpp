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

bool RayIntersectWall(
	const m_Vec2& v0, const m_Vec2& v1,
	float z_bottom, float z_top,
	const m_Vec3& ray_start_point,
	const m_Vec3& ray_direction_normalized,
	m_Vec3& out_pos );

bool RayIntersectXYPlane(
	float plane_z,
	const m_Vec3& ray_start_point,
	const m_Vec3& ray_direction_normalized,
	m_Vec3& out_pos );

bool RayIntersectCylinder(
	const m_Vec2& cylinder_center, float cylinder_radius,
	float cylinder_bottom, float cylinder_top,
	const m_Vec3& ray_start_point,
	const m_Vec3& ray_direction_normalized,
	m_Vec3& out_pos );

float DistanceToCylinder(
	const m_Vec2& cylinder_center, float cylinder_radius,
	float cylinder_bottom, float cylinder_top,
	const m_Vec3& pos );

} // namespace PanzerChasm
