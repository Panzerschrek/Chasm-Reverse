#pragma once
#include <list>
#include <string>

#include "commands_processor.hpp"
#include "log.hpp"
#include "system_event.hpp"
#include "time.hpp"

namespace PanzerChasm
{

class Console final
{
public:
	Console( CommandsProcessor& commands_processor, const SharedDrawersPtr& shared_drawers );
	~Console();

	void Toggle();
	bool IsActive() const;

	void ProcessEvents( const SystemEvents& events );
	void Draw();

private:
	struct UserMessage
	{
		std::string text;
		Time time= Time::FromSeconds(0);
	};

private:
	void RemoveOldUserMessages( Time current_time );
	void DrawUserMessages();

	void LogCallback( std::string str, Log::LogLevel log_level );

	void WriteHistory();
	void CopyLineFromHistory();

private:
	static constexpr unsigned int c_max_input_line_length= 64u;
	static constexpr unsigned int c_max_lines= 128u;
	static constexpr unsigned int c_max_history= 64u;

	CommandsProcessor& commands_processor_;
	const SharedDrawersPtr shared_drawers_;

	float position_= 0.0f;
	float current_speed_= -1.0f;
	Time last_draw_time_;

	std::list<std::string> lines_;
	unsigned int lines_position_= 0u;

	// TODO - Maybe use raw array or std::array, instead std::string.
	std::string history_[ c_max_history ];
	unsigned int history_size_= 0u;
	unsigned int next_history_line_index_= 0u;
	unsigned int current_history_line_= 0u; // From newer to older. 0 - current line without history.

	char input_line_[ c_max_input_line_length + 1u ];
	unsigned int input_cursor_pos_= 0u;

	std::list<UserMessage> user_messages_;
};

} // namespace PanzerChasm
