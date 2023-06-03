#pragma once
#include <cmath>
#include <limits>

#include <vec.hpp>

namespace PanzerChasm
{

namespace Constants
{

const float pi= 3.1415926535f;
const float inv_pi= 1.0f / pi;

const float half_pi= 0.5f * pi;
const float two_pi= 2.0f * pi;

const float to_rad= pi / 180.0f;
const float to_deg= 180.0f / pi;

const float max_float= std::numeric_limits<float>::max();
const float min_float= -max_float;

} // namespace Contants

// Returnz z angle in out_angle[0] and x angle in out_angle[1]
void VecToAngles( const m_Vec3& vec, float* out_angle );

// Convert to range [ 0; 2 * pi )
float NormalizeAngle( float angle );

float DistanceToLineSegment(
	const m_Vec2& from,
	const m_Vec2& v0,
	const m_Vec2& v1 );

} // namespace PanzerChasm


#if __cplusplus < 201703L
namespace std
{
	template<typename Tp_>
	constexpr inline Tp_ clamp(Tp_ dst, Tp_ lo, Tp_ hi)
	{
		return (dst <= hi ? (dst >= lo ? dst : lo) : hi);
	}
}
#endif
