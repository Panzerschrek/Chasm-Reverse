#pragma once
#include <list>
#include <string>

#include "commands_processor.hpp"
#include "drawers.hpp"
#include "system_event.hpp"
#include "time.hpp"

namespace PanzerChasm
{

class Console final
{
public:
	Console( CommandsProcessor& commands_processor, const DrawersPtr& drawers );
	~Console();

	void Toggle();
	bool IsActive() const;

	void ProcessEvents( const SystemEvents& events );
	void Draw();

private:
	void LogCallback( std::string str );

private:
	static constexpr unsigned int c_max_input_line_length= 64u;
	static constexpr unsigned int c_max_lines= 64u;

	CommandsProcessor& commands_processor_;
	const DrawersPtr drawers_;

	float position_= 0.0f;
	float current_speed_= -1.0f;
	Time last_draw_time_;

	std::list<std::string> lines_;

	char input_line_[ c_max_input_line_length + 1u ];
	unsigned int input_cursor_pos_= 0u;
};

} // namespace PanzerChasm
