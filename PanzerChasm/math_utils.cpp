#include "math_utils.hpp"

namespace PanzerChasm
{

void VecToAngles( const m_Vec3& vec, float* out_angle )
{
	out_angle[0]= std::atan2( vec.y, vec.x );
	out_angle[1]= std::atan2( vec.z, vec.xy().Length() );
}

float NormalizeAngle( const float angle )
{
	return
		std::fmod(
			std::fmod( angle, Constants::two_pi ) + Constants::two_pi,
			Constants::two_pi );
}

float DistanceToLineSegment(
	const m_Vec2& from,
	const m_Vec2& v0,
	const m_Vec2& v1 )
{
	const m_Vec2 v0_v1_vec= v1 - v0;
	const float v0_v1_vec_square_length= v0_v1_vec.SquareLength();

	const m_Vec2 vec_from_v0_to_pos= from - v0;
	const float vecs_dot= vec_from_v0_to_pos * v0_v1_vec;

	if( vecs_dot <= 0.0f )
	{
		return vec_from_v0_to_pos.Length();
	}
	else if( vecs_dot >= v0_v1_vec_square_length )
	{
		return ( from - v1 ).Length();
	}
	else
	{
		if( v0_v1_vec_square_length == 0.0f )
			return ( from - v0 ).Length();

		const m_Vec2 projection_to_line= v0_v1_vec * ( vecs_dot / v0_v1_vec_square_length );
		const m_Vec2 vec_from_line= vec_from_v0_to_pos - projection_to_line;

		return vec_from_line.Length();
	}

}

} // namespace PanzerChasm
