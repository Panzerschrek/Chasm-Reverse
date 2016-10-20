#pragma once
#include <SDL.h>

#include "size.hpp"
#include "system_event.hpp"

namespace PanzerChasm
{

// Main game window.
// Warning, multiple windows does not work correctly!
class SystemWindow final
{
public:
	SystemWindow();
	~SystemWindow();

	Size2 GetViewportSize() const;

	// Commit any draw operations and show frame.
	// May wait for vsync.
	void SwapBuffers();

	void GetInput( SystemEvents& out_events );

private:
	Size2 viewport_size_;

	SDL_Window* window_= nullptr;
	SDL_GLContext gl_context_= nullptr;
};

} // namespace PanzerChasm
