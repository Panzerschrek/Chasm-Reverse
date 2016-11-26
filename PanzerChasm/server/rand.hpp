#pragma once
#include <memory>
#include <random>

namespace PanzerChasm
{

class LongRand final
{
public:
	typedef unsigned int RandResultType;

	explicit LongRand( RandResultType seed= 0 );
	~LongRand();

	RandResultType Rand();

private:
	// Simple and fast generator.
	// Produces good result for bits 0-31.
	// Parameters, same as in rand() from glibc.
	typedef std::linear_congruential_engine< RandResultType, 1103515245u, 12345u, 1u << 31u > Generator;

public:
	static constexpr RandResultType c_max_rand_plus_one_= Generator::modulus;

private:
	Generator generator_;
};

typedef std::shared_ptr<LongRand> LongRandPtr;

} // namespace PanzerChasm
