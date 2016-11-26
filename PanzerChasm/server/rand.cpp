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

} // namespace PanzerChasm
