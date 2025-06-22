#pragma once
#include <bitset>
#include <vector>

namespace PanzerChasm
{

struct SystemEvent
{
	enum class Type
	{
		Key,
		CharInput,
		MouseKey,
		MouseMove,
		Wheel,
		Quit,
	};

	struct KeyEvent
	{
		enum KeyCode : char32_t
		{
			// Do not set here key names manually.
			Unknown= 0,

			Escape,
			Enter,
			Space,
			Backspace,
			Tab,

			PageUp,
			PageDown,

			Up,
			Down,
			Left,
			Right,
			BackQuote,

			A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
			K1, K2, K3, K4, K5, K6, K7, K8, K9, K0,

			// Put new keys at back.
			Minus, Equals,
			SquareBracketLeft, SquareBracketRight,
			Semicolon, Apostrophe, BackSlash,
			Comma, Period, Slash,

			F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

			KPDivide, KPMultiply, KPMinus, KPPlus, KPEnter,
			KP1, KP2, KP3, KP4, KP5, KP6, KP7, KP8, KP9, KP0, KPPeriod,

			Pause,
			CapsLock,
			LeftControl,
			RightControl,
			LeftAlt,
			RightAlt,
			LeftShift,
			RightShift,
			LeftMetaGUI,
			RightMetaGUI,
			Application,

			MouseUnknown, Mouse1, Mouse2, Mouse3, Mouse4, Mouse5, Mouse6,
			MouseWheelUp, MouseWheelDown,
			// Put it last here.
			KeyCount
		};

		enum Modifiers : char32_t
		{
			Shift= 0x01u,
			Control= 0x02u,
			Alt= 0x04u,
			Caps= 0x08u,
		};
		typedef char32_t ModifiersMask;

		KeyCode key_code;
		ModifiersMask modifiers;
		bool pressed;
	};

	struct CharInputEvent
	{
		char ch;
	};

	struct MouseKeyEvent
	{
		enum Button : char32_t
		{
			Unknown= 0,
			Left=1, Mouse1=1,
			Right=2, Mouse2=2,
			Middle=3, Mouse3=3,
			Mouse4=4,
			Mouse5=5,
			Mouse6=6,
			MouseWheelUp=7,
			MouseWheelDown=8,
			ButtonCount
		};

		Button mouse_button;
		unsigned int x, y;
		bool pressed;
	};

	struct MouseMoveEvent
	{
		int dx, dy;
	};

	struct WheelEvent
	{
		int delta;
	};

	struct QuitEvent
	{};

	Type type;
	union
	{
		KeyEvent key;
		MouseKeyEvent mouse_key;
		MouseMoveEvent mouse_move;
		WheelEvent wheel;
		QuitEvent quit;
		CharInputEvent char_input;
	} event;
};

typedef std::vector<SystemEvent> SystemEvents;

typedef std::bitset< static_cast<unsigned int>( SystemEvent::KeyEvent::KeyCode::KeyCount ) > KeyboardState;
typedef std::bitset< static_cast<unsigned int>( SystemEvent::MouseKeyEvent::Button::ButtonCount ) > MouseState;

struct InputState
{
	KeyboardState keyboard;
	MouseState mouse;
};

const char* GetKeyName( SystemEvent::KeyEvent::KeyCode key_code );
bool KeyCanBeUsedForControl( SystemEvent::KeyEvent::KeyCode key_code );

} // namespace PanzerChasm
