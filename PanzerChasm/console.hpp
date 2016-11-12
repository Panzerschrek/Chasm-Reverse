#pragma once
#include <list>
#include <string>

#include "commands_processor.hpp"
#include "system_event.hpp"

namespace PanzerChasm
{

class Console final
{
public:
	Console( CommandsProcessor& commands_processor );
	~Console();

	void ProcessEvents( const SystemEvents& events );

private:
	static constexpr unsigned int c_max_input_line_length= 64u;

	CommandsProcessor& commands_processor_;

	std::list<std::string> lines_;

	char input_line_[ c_max_input_line_length + 1u ];
	unsigned int input_cursor_pos_= 0u;
};

} // namespace PanzerChasm
