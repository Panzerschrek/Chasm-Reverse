#include "../assert.hpp"
#include "../math_utils.hpp"

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

} // namespace PanzerChasm
