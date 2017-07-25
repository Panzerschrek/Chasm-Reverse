#include <algorithm>
#include <cstring>

#include "assert.hpp"
#include "i_menu_drawer.hpp"
#include "i_text_drawer.hpp"
#include "log.hpp"
#include "shared_drawers.hpp"

#include "console.hpp"

namespace PanzerChasm
{

static const float g_user_message_show_time_s= 5.0f;
static const size_t g_max_user_messages= 4u;

Console::Console( CommandsProcessor& commands_processor, const SharedDrawersPtr& shared_drawers )
	: commands_processor_(commands_processor)
	, shared_drawers_(shared_drawers)
	, last_draw_time_(Time::CurrentTime())
{
	PC_ASSERT( shared_drawers_ != nullptr );

	input_line_[0]= '\0';

	Log::SetLogCallback( std::bind( &Console::LogCallback, this, std::placeholders::_1, std::placeholders::_2 ) );
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

	RemoveOldUserMessages( current_time );
	if( position_ <= 0.0f )
	{
		DrawUserMessages();
		return;
	}

	shared_drawers_->menu->DrawConsoleBackground( position_ );

	const int letter_height= int( shared_drawers_->text->GetLineHeight() );
	const int scale= int( shared_drawers_->menu->GetConsoleScale() );

	const unsigned int c_x_offset= 5u;
	int y= static_cast<int>(
		position_ * float( shared_drawers_->menu->GetViewportSize().Height() / 2u ) -
		float( letter_height * scale ) ) - 4 * scale;

	const bool draw_cursor= ( int( current_time.ToSeconds() * 3.0f ) & 1u ) != 0u;

	char line_with_cursor[ c_max_input_line_length + 3u ];
	std::snprintf( line_with_cursor, sizeof(line_with_cursor), draw_cursor ? ">%s_" : ">%s", input_line_ );

	shared_drawers_->text->Print(
		scale * c_x_offset, y,
		line_with_cursor, scale,
		ITextDrawer::FontColor::Golden, ITextDrawer::Alignment::Left );
	y-= ( letter_height + 2 ) * scale;

	if( lines_position_ > 0u )
	{
		const char* const str=
		"  ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^  ";

		shared_drawers_->text->Print(
			scale * c_x_offset, y,
			str, scale,
			ITextDrawer::FontColor::White, ITextDrawer::Alignment::Left );
		y-= letter_height * scale;
	}

	auto it= lines_.rbegin();
	it= std::next( it, std::min( lines_position_, static_cast<unsigned int>(lines_.size()) ) );
	for( ; it != lines_.rend(); ++it )
	{
		if( y < -letter_height * scale )
			break;

		shared_drawers_->text->Print(
			scale * c_x_offset, y,
			it->c_str(), scale,
			ITextDrawer::FontColor::White, ITextDrawer::Alignment::Left );
		y-= letter_height * scale;
	}
}

void Console::RemoveOldUserMessages( const Time current_time )
{
	while( user_messages_.size() > g_max_user_messages )
		user_messages_.pop_front();

	while( ! user_messages_.empty() &&
		( current_time - user_messages_.front().time ).ToSeconds() > g_user_message_show_time_s )
		user_messages_.pop_front();
}

void Console::DrawUserMessages()
{
	const int letter_height= int( shared_drawers_->text->GetLineHeight() );
	const int scale= int( shared_drawers_->menu->GetConsoleScale() );

	int y= 0;
	const int c_offset= 5;
	for( const UserMessage& message : user_messages_ )
	{
		shared_drawers_->text->Print(
			scale * c_offset, y,
			message.text.c_str(), scale,
			ITextDrawer::FontColor::YellowGreen, ITextDrawer::Alignment::Left );

		y+= letter_height * scale;
	}
}

void Console::LogCallback( std::string str, const Log::LogLevel log_level )
{
	lines_.emplace_back( std::move(str) );

	if( lines_.size() == c_max_lines )
		lines_.pop_front();

	lines_position_= 0u;

	if( log_level == Log::LogLevel::User )
	{
		user_messages_.emplace_back();
		user_messages_.back().text= lines_.back();
		user_messages_.back().time= Time::CurrentTime();
	}
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
