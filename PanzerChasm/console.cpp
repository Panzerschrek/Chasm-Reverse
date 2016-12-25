#include <cstring>

#include "assert.hpp"
#include "drawers.hpp"
#include "log.hpp"

#include "console.hpp"

namespace PanzerChasm
{

Console::Console( CommandsProcessor& commands_processor, const DrawersPtr& drawers )
	: commands_processor_(commands_processor)
	, drawers_(drawers)
	, last_draw_time_(Time::CurrentTime())
{
	input_line_[0]= '\0';

	Log::SetLogCallback( std::bind( &Console::LogCallback, this, std::placeholders::_1 ) );
}

Console::~Console()
{
	Log::SetLogCallback( nullptr );
}

void Console::Toggle()
{
	current_speed_= current_speed_ > 0.0f ? -1.0f : 1.0f;
}

bool Console::IsActive() const
{
	return current_speed_ > 0.0f;
}

void Console::ProcessEvents( const SystemEvents& events )
{
	using KeyCode= SystemEvent::KeyEvent::KeyCode;

	for( const SystemEvent& event : events )
	{
		if( event.type == SystemEvent::Type::CharInput )
		{
			if( event.event.char_input.ch != '`' &&
				input_cursor_pos_ < c_max_input_line_length )
			{
				input_line_[ input_cursor_pos_ ]= event.event.char_input.ch;
				input_cursor_pos_++;
				input_line_[ input_cursor_pos_ ]= '\0';
			}
			continue;
		}

		if( !( event.type == SystemEvent::Type::Key && event.event.key.pressed ) )
			continue;

		const KeyCode key_code= event.event.key.key_code;
		if( key_code == KeyCode::Enter )
		{
			lines_position_= 0u;
			Log::Info( input_line_ );

			if( input_cursor_pos_ > 0u )
			{
				WriteHistory();
				commands_processor_.ProcessCommand( input_line_ );
			}

			input_cursor_pos_= 0u;
			input_line_[ input_cursor_pos_ ]= '\0';
		}
		else if( key_code == KeyCode::Escape )
		{
			Toggle();
		}
		else if( key_code == KeyCode::Backspace )
		{
			if( input_cursor_pos_ > 0u )
			{
				input_cursor_pos_--;
				input_line_[ input_cursor_pos_ ]= '\0';
			}
		}
		else if( key_code == KeyCode::Tab )
		{
			const std::string completed_command= commands_processor_.TryCompleteCommand( input_line_ );
			std::strncpy( input_line_, completed_command.c_str(), sizeof(input_line_) );
			input_cursor_pos_= std::strlen( input_line_ );
		}
		else if( key_code == KeyCode::Up )
		{
			if( history_size_ > 0u )
			{
				if( current_history_line_ < history_size_ && current_history_line_ < c_max_history )
					current_history_line_++;
				CopyLineFromHistory();
			}
		}
		else if( key_code == KeyCode::Down )
		{
			if( current_history_line_ > 0u )
			{
				current_history_line_--;

				if( current_history_line_ > 0u && history_size_ > 0u )
					CopyLineFromHistory();
			}
			if( current_history_line_ == 0u )
			{
				// Reset current line
				input_line_[0]= '\0';
				input_cursor_pos_= 0;
			}
		}
		else if( key_code == KeyCode::PageUp )
		{
			lines_position_+= 3u;
			lines_position_= std::min( lines_position_, static_cast<unsigned int>(lines_.size()) );
		}
		else if( key_code == KeyCode::PageDown )
		{
			if( lines_position_ > 3u ) lines_position_-= 3u;
			else lines_position_= 0u;
		}

	} // for events
}

void Console::Draw()
{
	const Time current_time= Time::CurrentTime();
	const float time_delta_s= ( current_time - last_draw_time_ ).ToSeconds();
	last_draw_time_= current_time;

	position_+= current_speed_ * time_delta_s;
	position_= std::min( 1.0f, std::max( 0.0f, position_ ) );

	if( position_ <= 0.0f )
		return;

	drawers_->menu.DrawConsoleBackground( position_ );

	const int letter_height= int( drawers_->text.GetLineHeight() );

	const unsigned int c_x_offset= 5u;
	int y= static_cast<int>(
		position_ * float( drawers_->menu.GetViewportSize().Height() / 2u ) -
		float( letter_height ) ) - 4u;

	const bool draw_cursor= ( int( current_time.ToSeconds() * 3.0f ) & 1u ) != 0u;

	char line_with_cursor[ c_max_input_line_length + 3u ];
	std::snprintf( line_with_cursor, sizeof(line_with_cursor), draw_cursor ? ">%s_" : ">%s", input_line_ );

	drawers_->text.Print(
		c_x_offset, y,
		line_with_cursor, 1,
		TextDraw::FontColor::Golden, TextDraw::Alignment::Left );
	y-= letter_height + 2u;

	if( lines_position_ > 0u )
	{
		const char* const str=
		"  ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^  ";

		drawers_->text.Print(
			c_x_offset, y,
			str, 1,
			TextDraw::FontColor::White, TextDraw::Alignment::Left );
		y-= letter_height;
	}

	auto it= lines_.rbegin();
	it= std::next( it, std::min( lines_position_, static_cast<unsigned int>(lines_.size()) ) );
	for( ; it != lines_.rend(); ++it )
	{
		if( y < -letter_height )
			break;

		drawers_->text.Print(
			c_x_offset, y,
			it->c_str(), 1,
			TextDraw::FontColor::White, TextDraw::Alignment::Left );
		y-= letter_height;
	}
}

void Console::LogCallback( std::string str )
{
	lines_.emplace_back( std::move(str) );

	if( lines_.size() == c_max_lines )
		lines_.pop_front();

	lines_position_= 0u;
}

void Console::WriteHistory()
{
	history_[ next_history_line_index_ ]= input_line_;
	next_history_line_index_= ( next_history_line_index_ + 1u ) % c_max_history;
	history_size_++;
	current_history_line_= 0u;
}

void Console::CopyLineFromHistory()
{
	PC_ASSERT( history_size_ > 0u );

	std::strncpy(
		input_line_,
		history_[ ( history_size_ - current_history_line_ ) % c_max_history ].c_str(),
		sizeof(input_line_) );

	input_cursor_pos_= std::strlen( input_line_ );
}

} // namespace PanzerChasm
