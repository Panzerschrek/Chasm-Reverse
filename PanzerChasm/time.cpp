#include <chrono>
#include <cmath>

#include "time.hpp"

namespace PanzerChasm
{

Time Time::CurrentTime()
{
	const auto current_time= std::chrono::steady_clock::now();

	const auto now_points=
		std::chrono::duration_cast<
			std::chrono::duration<
				int64_t,
				std::ratio<1, c_units_in_second> > >( current_time.time_since_epoch() );

	return Time( now_points.count() );
}

Time Time::FromSeconds( double seconds )
{
	return
		Time(
			static_cast<int64_t>(
				std::round( seconds *
				static_cast<double>(c_units_in_second) ) ) );
}

Time Time::FromSeconds( int seconds )
{
	return FromSeconds( int64_t( seconds ) );
}

Time Time::FromSeconds( int64_t seconds )
{
	return Time( seconds * c_units_in_second );
}

Time Time::FromInternalRepresentation( const int64_t r )
{
	return Time( r );
}

float Time::ToSeconds() const
{
	return
		static_cast<float>( time_ ) /
		static_cast<float>( c_units_in_second );
}

int64_t Time::GetInternalRepresentation() const
{
	return time_;
}

Time Time::operator+( const Time& other ) const
{
	return Time( time_ + other.time_ );
}

Time Time::operator-( const Time& other ) const
{
	return Time( time_ - other.time_ );
}

Time& Time::operator+=( const Time& other )
{
	time_+= other.time_;
	return *this;
}

Time& Time::operator-=( const Time& other )
{
	time_-= other.time_;
	return *this;
}

bool Time::operator==( const Time& other ) const
{
	return time_ == other.time_;
}

bool Time::operator!=( const Time& other ) const
{
	return time_ != other.time_;
}

bool Time::operator< ( const Time& other ) const
{
	return time_ <  other.time_;
}

bool Time::operator<=( const Time& other ) const
{
	return time_ <= other.time_;
}

bool Time::operator> ( const Time& other ) const
{
	return time_ >  other.time_;
}

bool Time::operator>=( const Time& other ) const
{
	return time_ >= other.time_;
}

Time::Time( const int64_t points )
	: time_(points)
{}

} // namespace PanzerChasm
