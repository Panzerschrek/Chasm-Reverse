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
