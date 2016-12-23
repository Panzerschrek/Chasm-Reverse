#include "assert.hpp"
#include "math_utils.hpp"

#include "rand.hpp"

namespace PanzerChasm
{

LongRand::LongRand( const RandResultType seed )
	: generator_(seed)
{}

LongRand::~LongRand()
{}

LongRand::RandResultType LongRand::Rand()
{
	return generator_();
}

float LongRand::RandAngle()
{
	return float( Rand() ) / float(c_max_rand_plus_one_) * Constants::two_pi;
}

float LongRand::RandValue( const float next_value_after_max )
{
	return float( double(Rand()) / double(c_max_rand_plus_one_) * double(next_value_after_max) );
}

float LongRand::RandValue( const float min_value, const float next_value_after_max )
{
	PC_ASSERT( min_value <= next_value_after_max );

	return RandValue( next_value_after_max - min_value ) + min_value;
}

m_Vec3 LongRand::RandPointInSphere( const float sphere_radius )
{
	m_Vec3 result;

	const float square_sphere_radius= sphere_radius * sphere_radius;

	// Generate random value in cube.
	// continue, while value is outside sphere.
	while(1)
	{
		for( unsigned int j= 0u; j < 3u; j++ )
			result.ToArr()[j]= RandValue( -sphere_radius, sphere_radius );

		const float square_length= result.SquareLength();
		if( square_length < square_sphere_radius )
			break;
	}

	return result;
}

m_Vec3 LongRand::RandDirection()
{
	m_Vec3 result;

	while(1)
	{
		result= RandPointInSphere( 1.0f );
		const float result_square_length= result.SquareLength();
		if( result_square_length == 0.0f )
			continue;

		result/= std::sqrt( result_square_length );
		break;
	}

	return result;
}

} // namespace PanzerChasm
