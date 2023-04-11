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
		enum class KeyCode
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
			K0, K1, K2, K3, K4, K5, K6, K7, K8, K9,

			// Put new keys at back.
			Minus, Equals,
			SquareBrackretLeft, SquareBrackretRight,
			Semicolon, Apostrophe, BackSlash,
			Comma, Period, Slash,

			F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

			Pause,
			// Put it last here.
			KeyCount
		};

		enum Modifiers : unsigned int
		{
			Shift= 0x01u,
			Control= 0x02u,
			Alt= 0x04u,
			Caps= 0x08u,
		};
		typedef unsigned int ModifiersMask;

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
		enum class Button
		{
			Unknown= 0,
			Left, Right, Middle,
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
		unsigned int timestamp;
		unsigned int window_id;
		unsigned int mouse_id; 
		int mouse_x, mouse_y;
		int x, y;
		float dx, dy;
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
