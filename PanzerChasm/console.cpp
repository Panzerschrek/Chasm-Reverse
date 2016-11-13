#include "log.hpp"

#include "console.hpp"

namespace PanzerChasm
{

static char KeyCodeToChar( const SystemEvent::KeyEvent::KeyCode code )
{
	using KeyCode= SystemEvent::KeyEvent::KeyCode;
	switch( code )
	{
	case KeyCode::Space: return ' ';

	case KeyCode::Escape:
	case KeyCode::Enter:
	case KeyCode::Backspace:
	case KeyCode::Up:
	case KeyCode::Down:
	case KeyCode::Left:
	case KeyCode::Right:
		break;

	default:
		if( code >= KeyCode::A && code <= KeyCode::Z )
			return static_cast<int>(code) - static_cast<int>(KeyCode::A) + 'a';
		if( code >= KeyCode::K0 && code <= KeyCode::K9 )
			return static_cast<int>(code) - static_cast<int>(KeyCode::K0) + '0';
		break;
	}

	return '\0';
}

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
		if( !( event.type == SystemEvent::Type::Key && event.event.key.pressed ) )
			continue;

		const KeyCode key_code= event.event.key.key_code;
		if( key_code == KeyCode::Enter )
		{
			Log::Info( input_line_ );
			commands_processor_.ProcessCommand( input_line_ );

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
		else if(
			key_code == KeyCode::Up || key_code == KeyCode::Down ||
			key_code == KeyCode::Left || key_code == KeyCode::Right )
		{
		}
		else
		{
			if( input_cursor_pos_ < c_max_input_line_length )
			{
				const char ch= KeyCodeToChar( key_code );
				if( ch != '\0' )
				{
					input_line_[ input_cursor_pos_ ]= ch;
					input_cursor_pos_++;
					input_line_[ input_cursor_pos_ ]= '\0';
				}
			}
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

	const int letter_height= int( drawers_->text.GetLineHeight() );

	const int input_line_y= static_cast<int>(
		position_ * float( drawers_->menu.GetViewportSize().Height() / 2u ) -
		float( letter_height ) );

	const bool draw_cursor= ( int( current_time.ToSeconds() * 3.0f ) & 1u ) != 0u;

	char line_with_cursor[ c_max_input_line_length + 2u ];
	std::snprintf( line_with_cursor, sizeof(line_with_cursor), draw_cursor ? "%s_" : "%s", input_line_ );

	drawers_->text.Print(
		0, input_line_y,
		line_with_cursor, 1,
		TextDraw::FontColor::Golden, TextDraw::Alignment::Left );

	int i= 1u;
	for( auto it= lines_.rbegin(); it != lines_.rend(); ++it, i++ )
	{
		const int y= input_line_y - i * letter_height;
		if( y < -letter_height )
			break;

		drawers_->text.Print(
			0, y,
			it->c_str(), 1,
			TextDraw::FontColor::White, TextDraw::Alignment::Left );
	}
}

void Console::LogCallback( std::string str )
{
	lines_.emplace_back( std::move(str) );

	if( lines_.size() == c_max_lines )
		lines_.pop_front();
}

} // namespace PanzerChasm
