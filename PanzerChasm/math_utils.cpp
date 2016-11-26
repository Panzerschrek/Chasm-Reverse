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

} // namespace PanzerChasm
