#pragma once
#include <cstdint>

namespace PanzerChasm
{

// Time class.
// Internal represintation is integer, therefore there are no precision lost in time delta calculations.
class Time final
{
public:
	static Time CurrentTime();

	static Time FromSeconds( double seconds );
	static Time FromSeconds( int seconds );
	static Time FromSeconds( int64_t seconds );
	static Time FromInternalRepresentation( int64_t r );

public:
	Time( const Time& other )= default;
	Time& operator=( const Time& other )= default;

	float ToSeconds() const;
	int64_t GetInternalRepresentation() const;

	Time operator+( const Time& other ) const;
	Time operator-( const Time& other ) const;

	Time& operator+=( const Time& other );
	Time& operator-=( const Time& other );

	bool operator==( const Time& other ) const;
	bool operator!=( const Time& other ) const;
	bool operator< ( const Time& other ) const;
	bool operator<=( const Time& other ) const;
	bool operator> ( const Time& other ) const;
	bool operator>=( const Time& other ) const;

	// Upscale time.
	Time operator*( unsigned int scale ) const;

private:
	Time()= delete;
	explicit Time( int64_t points );

private:
	static constexpr int64_t c_units_in_second= 10000u;

	int64_t time_;
};

// Upscale time.
Time operator*( unsigned int scale, const Time& time );

} // namespace PanzerChasm
