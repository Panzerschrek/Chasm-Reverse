#pragma once
#include <vector>

namespace PanzerChasm
{

struct SystemEvent
{
	enum class Type
	{
		Key,
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

			Up,
			Down,
			Left,
			Right,

			A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
			K0, K1, K2, K3, K4, K5, K6, K7, K8, K9,
		};

		KeyCode key_code;
		bool pressed;
	};

	struct MouseKeyEvent
	{
		unsigned int mouse_button;
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
	} event;
};

typedef std::vector<SystemEvent> SystemEvents;

} // namespace PanzerChasm
