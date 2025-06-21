#include <cmath>

#include "math_utils.hpp"

#include "messages.hpp"

namespace PanzerChasm
{

Messages::CoordType CoordToMessageCoord( const float x )
{
	return static_cast<Messages::CoordType>( std::round( x * 256.0f ) );
}

float MessageCoordToCoord( const Messages::CoordType x )
{
	return float(x) / 256.0f;
}

void PositionToMessagePosition( const m_Vec2& pos, Messages::CoordType* out_pos )
{
	for( unsigned int i= 0u; i < 2u; i++ )
		out_pos[i]= CoordToMessageCoord( pos.ToArr()[i] );
}

void PositionToMessagePosition( const m_Vec3& pos, Messages::CoordType* out_pos )
{
	for( unsigned int i= 0u; i < 3u; i++ )
		out_pos[i]= CoordToMessageCoord( pos.ToArr()[i] );
}

void MessagePositionToPosition( const Messages::CoordType* pos, m_Vec2& out_pos )
{
	for( unsigned int i= 0u; i < 2u; i++ )
		out_pos.ToArr()[i]= MessageCoordToCoord( pos[i] );
}

void MessagePositionToPosition( const Messages::CoordType* pos, m_Vec3& out_pos )
{
	for( unsigned int i= 0u; i < 3u; i++ )
		out_pos.ToArr()[i]= MessageCoordToCoord( pos[i] );
}

Messages::AngleType AngleToMessageAngle( const float angle )
{
	return static_cast<Messages::AngleType>( NormalizeAngle( angle ) * 65536.0f / Constants::two_pi );
}

float MessageAngleToAngle( const Messages::AngleType angle )
{
	return float(angle) / 65536.0f * Constants::two_pi;
}

} // namespace PanzerChasm
