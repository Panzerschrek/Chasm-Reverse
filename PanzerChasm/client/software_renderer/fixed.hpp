#pragma once
#include <cstdint>
#include <limits>

#include "../../assert.hpp"

namespace PanzerChasm
{

typedef std::int32_t fixed_base_t;
typedef std::int64_t fixed_base_square_t;

#define ASSERT_INVALID_BASE \
	static_assert( base >= 0 && base <= std::numeric_limits<fixed_base_t>::digits, "Invalid fixed base" );

template< int base >
fixed_base_t FixedOne()
{
	ASSERT_INVALID_BASE
	return 1 << base;
}

template< int base >
fixed_base_t FixedHalf()
{
	ASSERT_INVALID_BASE
	return 1 << ( base - 1);
}

template< int base >
fixed_base_t FixedMul( fixed_base_t x, fixed_base_t y )
{
	ASSERT_INVALID_BASE
	return ( fixed_base_square_t(x) * y ) >> base;
}

template< int base >
fixed_base_t FixedSquare( fixed_base_t x )
{
	ASSERT_INVALID_BASE
	return ( fixed_base_square_t(x) * x ) >> base;
}

template< int base >
int FixedMulResultToInt( fixed_base_t x, fixed_base_t y )
{
	ASSERT_INVALID_BASE
	return ( fixed_base_square_t(x) * y ) >> ( fixed_base_square_t(base) * 2 );
}

template< int base >
fixed_base_t FixedRound( fixed_base_t x )
{
	ASSERT_INVALID_BASE
	return ( x + (1 << (base - 1)) ) & (~( (1 << base) - 1 ));
}

template< int base >
int FixedRoundToInt( fixed_base_t x )
{
	ASSERT_INVALID_BASE
	return ( x + (1 << (base - 1)) ) >> base;
}

template< int base >
fixed_base_t FixedDiv( fixed_base_t x, fixed_base_t y )
{
	ASSERT_INVALID_BASE
	// TODO - check range of result
	PC_ASSERT( y != 0 );
	return ( fixed_base_square_t(x) << base ) / y;
}

template< int base >
fixed_base_t FixedInvert( fixed_base_t x )
{
	ASSERT_INVALID_BASE
	return ( fixed_base_square_t(1) << ( fixed_base_square_t(base) * 2 ) ) / fixed_base_square_t(x);
}

// If you wish - you can add fixedXX_t and additional direct functions.
typedef fixed_base_t fixed16_t;
typedef fixed_base_t fixed8_t;

const fixed16_t g_fixed16_one= FixedOne<16>();
const fixed16_t g_fixed16_half= FixedHalf<16>();

inline fixed16_t Fixed16Mul( fixed16_t x, fixed16_t y )
{
	return FixedMul<16>( x, y );
}

inline fixed16_t Fixed16Square( fixed16_t x )
{
	return FixedSquare<16>( x );
}

inline int Fixed16MulResultToInt( fixed16_t x, fixed16_t y )
{
	return FixedMulResultToInt<16>( x, y );
}

inline fixed16_t Fixed16Round( fixed16_t x )
{
	return FixedRound<16>( x );
}

inline int Fixed16RoundToInt( fixed16_t x )
{
	return FixedRoundToInt<16>( x );
}

inline fixed16_t Fixed16Div( fixed16_t x, fixed16_t y )
{
	return FixedDiv<16>( x, y );
}

inline fixed16_t Fixed16Invert( fixed16_t x )
{
	return FixedInvert<16>( x );
}

#undef ASSERT_INVALID_BASE

} // namespace PanzerChasm
