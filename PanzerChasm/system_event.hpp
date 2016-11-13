#pragma once
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
			Unknown= 0,
			Escape,
			Enter,
			Space,
			Backspace,

			PageUp,
			PageDown,

			Up,
			Down,
			Left,
			Right,
			BackQuote,

			A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
			K0, K1, K2, K3, K4, K5, K6, K7, K8, K9,
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
		unsigned int mouse_button;
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
		MouseMoveEvent mosue_move;
		QuitEvent quit;
		CharInputEvent char_input;
	} event;
};

typedef std::vector<SystemEvent> SystemEvents;

} // namespace PanzerChasm
