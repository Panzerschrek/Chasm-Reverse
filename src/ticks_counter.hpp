#pragma once
#include "time.hpp"

namespace PanzerChasm
{

class TicksCounter final
{
public:
	TicksCounter( Time frequency_calc_interval= Time::FromSeconds(1) );
	~TicksCounter();

	void Tick( unsigned int count = 1u );
	float GetTicksFrequency() const;
	unsigned int GetTotalTicks() const;

private:
	TicksCounter& operator=(const TicksCounter&)= delete;

private:
	const Time frequency_calc_interval_;

	unsigned int total_ticks_;
	float output_ticks_frequency_;
	unsigned int current_sample_ticks_;

	Time last_update_time_; // Real time
};

} // namespace PanzerChasm
