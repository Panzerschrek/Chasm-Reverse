#include <cmath>

#include "ticks_counter.hpp"

namespace PanzerChasm
{

TicksCounter::TicksCounter( const Time frequency_calc_interval )
	: frequency_calc_interval_(frequency_calc_interval)
	, total_ticks_(0u)
	, output_ticks_frequency_(0.0f)
	, current_sample_ticks_(0u)
	, last_update_time_( Time::CurrentTime() )
{}

TicksCounter::~TicksCounter()
{}

void TicksCounter::Tick( const unsigned int count )
{
	total_ticks_+= count;
	current_sample_ticks_+= count;

	const Time current_time= Time::CurrentTime();
	const Time dt= current_time - last_update_time_;

	if( dt >= frequency_calc_interval_ )
	{
		output_ticks_frequency_= float(current_sample_ticks_) / frequency_calc_interval_.ToSeconds();
		current_sample_ticks_= 0u;
		last_update_time_+= dt;
	}
}

float TicksCounter::GetTicksFrequency() const
{
	return output_ticks_frequency_;
}

unsigned int TicksCounter::GetTotalTicks() const
{
	return total_ticks_;
}

} // namespace PanzerChasm
