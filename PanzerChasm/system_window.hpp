#pragma once
#include <SDL.h>

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

	unsigned int Width () const;
	unsigned int Height() const;

	// Commit any draw operations and show frame.
	// May wait for vsync.
	void SwapBuffers();

	void GetInput( SystemEvents& out_events );

private:
	unsigned int viewport_size_[2];

	SDL_Window* window_= nullptr;
	SDL_GLContext gl_context_= nullptr;
};

} // namespace PanzerChasm
