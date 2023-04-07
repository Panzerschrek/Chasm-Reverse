#pragma once
#include <memory>
#include <random>

#include <vec.hpp>

namespace PanzerChasm
{

class LongRand final
{
public:
	typedef unsigned int RandResultType;

	explicit LongRand( RandResultType seed= 0 );
	~LongRand();

	RandResultType Rand();

	// Random angle in range [ 0; 2 * pi ) with linear distribution.
	float RandAngle();
	// Random value in range [ 0; next_value_after_max ) with linear distribution.
	float RandValue( float next_value_after_max );
	// Random value in range [ min_value; next_value_after_max ) with linear distribution.
	float RandValue( float min_value, float next_value_after_max );

	// Random value in sphere with linear density.
	m_Vec3 RandPointInSphere( float sphere_radius );
	// Normalized point on identity sphere.
	m_Vec3 RandDirection();

	// Returns "true" with chance= 0.5.
	bool RandBool();
	// Returns "true" with chanse= 1 / denominator.
	bool RandBool( RandResultType chance_denominator );
	// Returns "true" with chanse= chance_numerator / denominator.
	bool RandBool( RandResultType chance_numerator, RandResultType chance_denominator );

	// Serialization.
	uint32_t GetInnerState() const;
	void SetInnerState( uint32_t state );

private:
	// Simple and fast generator.
	// Produces good result for bits 0-31.
	// Parameters, same as in rand() from glibc.
	typedef std::linear_congruential_engine< RandResultType, 1103515245u, 12345u, 1u << 31u > Generator;
	typedef long long unsigned int ExtendedType;

	static_assert(
		sizeof(uint32_t) >= sizeof(Generator),
		"Generator is too big. You need rewrite serialization methods" );

public:
	static constexpr RandResultType c_max_rand_plus_one_= Generator::modulus;

private:
	Generator generator_;
};

} // namespace PanzerChasm
