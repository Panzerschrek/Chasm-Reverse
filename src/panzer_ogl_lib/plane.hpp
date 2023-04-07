#pragma once
#include "vec.hpp"

class m_Plane2
{
public:
	m_Vec2 normal; // Must be normalized
	float dist; // Distance from center

	/* For point on plane  P * normal + dist == 0
	 * For point ahead plane  P * normal + dist > 0
	 * For point behind plane  P * normal + dist < 0
	*/

public:
	m_Plane2(){}
	m_Plane2( const m_Vec2& in_normal, float in_dist );

	bool IsPointAheadPlane( const m_Vec2& point ) const;
	bool IsPointBehindPlane( const m_Vec2& point ) const;

	float GetSignedDistance(const m_Vec2& point ) const;

	// TODO - add other useful methods - rotation, matrix transformations, etc.
};


class m_Plane3
{
public:
	m_Vec3 normal; // Must be normalized
	float dist; // Distance from center

	/* For point on plane  P * normal + dist == 0
	 * For point ahead plane  P * normal + dist > 0
	 * For point behind plane  P * normal + dist < 0
	*/

public:
	m_Plane3(){}
	m_Plane3( const m_Vec3& in_normal, float in_dist );

	bool IsPointAheadPlane( const m_Vec3& point ) const;
	bool IsPointBehindPlane( const m_Vec3& point ) const;

	float GetSignedDistance(const m_Vec3& point ) const;

	// TODO - add other useful methods - rotation, matrix transformations, etc.
};

// m_Plane2 inline

inline m_Plane2::m_Plane2( const m_Vec2& in_normal, const float in_dist )
	: normal( in_normal ), dist( in_dist )
{}

inline bool m_Plane2::IsPointAheadPlane( const m_Vec2& point ) const
{
	return point * normal + dist > 0.0f;
}

inline bool m_Plane2::IsPointBehindPlane( const m_Vec2& point ) const
{
	return point * normal + dist < 0.0f;
}

inline float m_Plane2::GetSignedDistance(const m_Vec2& point ) const
{
	return point * normal + dist;
}

// m_Plane3 inline

inline m_Plane3::m_Plane3( const m_Vec3& in_normal, const float in_dist )
	: normal( in_normal ), dist( in_dist )
{}

inline bool m_Plane3::IsPointAheadPlane( const m_Vec3& point ) const
{
	return point * normal + dist > 0.0f;
}

inline bool m_Plane3::IsPointBehindPlane( const m_Vec3& point ) const
{
	return point * normal + dist < 0.0f;
}

inline float m_Plane3::GetSignedDistance(const m_Vec3& point ) const
{
	return point * normal + dist;
}
