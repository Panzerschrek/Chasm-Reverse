#include "assert.hpp"

#include "system_event.hpp"

namespace PanzerChasm
{

const char* GetKeyName( const SystemEvent::KeyEvent::KeyCode key_code )
{
	using KeyCode= SystemEvent::KeyEvent::KeyCode;
	switch( key_code )
	{
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
	};

	PC_ASSERT(false);
	return "";
}

} // namespace PanzerChasm
