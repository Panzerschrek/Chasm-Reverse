#include "assert.hpp"

#include "system_event.hpp"

namespace PanzerChasm
{

const char* GetKeyName( const SystemEvent::KeyEvent::KeyCode key_code )
{
	using KeyCode= SystemEvent::KeyEvent::KeyCode;
	switch( key_code )
	{
	case KeyCode::KeyCount:
	case KeyCode::Unknown: return "";

	case KeyCode::Escape: return "Esc";
	case KeyCode::Enter: return "Enter";
	case KeyCode::Space: return "Space";
	case KeyCode::Backspace: return "Backspace";
	case KeyCode::Tab: return "Tab";

	case KeyCode::PageUp: return "PageUp";
	case KeyCode::PageDown: return "PageDown";

	case KeyCode::Up: return "Up";
	case KeyCode::Down: return "Down";
	case KeyCode::Left: return "Left";
	case KeyCode::Right: return "Right";
	case KeyCode::BackQuote: return "`";

	case KeyCode::A: return "A";
	case KeyCode::B: return "B";
	case KeyCode::C: return "C";
	case KeyCode::D: return "D";
	case KeyCode::E: return "E";
	case KeyCode::F: return "F";
	case KeyCode::G: return "G";
	case KeyCode::H: return "H";
	case KeyCode::I: return "I";
	case KeyCode::J: return "J";
	case KeyCode::K: return "K";
	case KeyCode::L: return "L";
	case KeyCode::M: return "M";
	case KeyCode::N: return "N";
	case KeyCode::O: return "O";
	case KeyCode::P: return "P";
	case KeyCode::Q: return "Q";
	case KeyCode::R: return "R";
	case KeyCode::S: return "S";
	case KeyCode::T: return "T";
	case KeyCode::U: return "U";
	case KeyCode::V: return "V";
	case KeyCode::W: return "W";
	case KeyCode::X: return "X";
	case KeyCode::Y: return "X";
	case KeyCode::Z: return "Z";

	case KeyCode::K0: return "0";
	case KeyCode::K1: return "1";
	case KeyCode::K2: return "2";
	case KeyCode::K3: return "3";
	case KeyCode::K4: return "4";
	case KeyCode::K5: return "5";
	case KeyCode::K6: return "6";
	case KeyCode::K7: return "7";
	case KeyCode::K8: return "8";
	case KeyCode::K9: return "9";

	case KeyCode::Minus: return "-";
	case KeyCode::Equals: return "=";

	case KeyCode::SquareBrackretLeft: return "[";
	case KeyCode::SquareBrackretRight: return "]";

	case KeyCode::Semicolon: return ";";
	case KeyCode::Apostrophe: return "'";
	case KeyCode::BackSlash: return "\\";

	case KeyCode::Comma: return ",";
	case KeyCode::Period: return ".";
	case KeyCode::Slash: return "/";

	case KeyCode::F1: return "F1";
	case KeyCode::F2: return "F2";
	case KeyCode::F3: return "F3";
	case KeyCode::F4: return "F4";
	case KeyCode::F5: return "F5";
	case KeyCode::F6: return "F6";
	case KeyCode::F7: return "F7";
	case KeyCode::F8: return "F8";
	case KeyCode::F9: return "F9";
	case KeyCode::F10: return "F10";
	case KeyCode::F11: return "F11";
	case KeyCode::F12: return "F12";

	case KeyCode::Pause: return "Pause";
	};

	PC_ASSERT(false);
	return "";
}

bool KeyCanBeUsedForControl( const SystemEvent::KeyEvent::KeyCode key_code )
{
	using KeyCode= SystemEvent::KeyEvent::KeyCode;
	switch( key_code )
	{
	case KeyCode::Enter:
	case KeyCode::Space:

	case KeyCode::PageUp:
	case KeyCode::PageDown:

	case KeyCode::Up:
	case KeyCode::Down:
	case KeyCode::Left:
	case KeyCode::Right:
	case KeyCode::BackQuote:

	case KeyCode::A:
	case KeyCode::B:
	case KeyCode::C:
	case KeyCode::D:
	case KeyCode::E:
	case KeyCode::F:
	case KeyCode::G:
	case KeyCode::H:
	case KeyCode::I:
	case KeyCode::J:
	case KeyCode::K:
	case KeyCode::L:
	case KeyCode::M:
	case KeyCode::N:
	case KeyCode::O:
	case KeyCode::P:
	case KeyCode::Q:
	case KeyCode::R:
	case KeyCode::S:
	case KeyCode::T:
	case KeyCode::U:
	case KeyCode::V:
	case KeyCode::W:
	case KeyCode::X:
	case KeyCode::Y:
	case KeyCode::Z:

	case KeyCode::Semicolon:
	case KeyCode::Apostrophe:
	case KeyCode::BackSlash:
	case KeyCode::Comma:
	case KeyCode::Period:
	case KeyCode::Slash:

		return true;

	case KeyCode::Escape: // menu navigation key
	case KeyCode::Backspace:
	case KeyCode::Tab: // minimap

	case KeyCode::K0: // Numbers keys used for weapon select
	case KeyCode::K1:
	case KeyCode::K2:
	case KeyCode::K3:
	case KeyCode::K4:
	case KeyCode::K5:
	case KeyCode::K6:
	case KeyCode::K7:
	case KeyCode::K8:
	case KeyCode::K9:

	case KeyCode::Minus: // +- used for hud scaling
	case KeyCode::Equals:

	case KeyCode::F1: // Functional keys used for quick commands
	case KeyCode::F2:
	case KeyCode::F3:
	case KeyCode::F4:
	case KeyCode::F5:
	case KeyCode::F6:
	case KeyCode::F7:
	case KeyCode::F8:
	case KeyCode::F9:
	case KeyCode::F10:
	case KeyCode::F11:
	case KeyCode::F12:

	case KeyCode::Pause: // Pause key used for pause (C.O.)

	case KeyCode::Unknown:
	case KeyCode::KeyCount:

		return false;
	};

	return false;
}

} // namespace PanzerChasm
