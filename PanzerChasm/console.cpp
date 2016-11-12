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

Console::Console( CommandsProcessor& commands_processor )
	: commands_processor_(commands_processor)
{
	input_line_[0]= '\0';
}

Console::~Console()
{}

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

} // namespace PanzerChasm
