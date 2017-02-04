#pragma once
#include <SDL.h>

#include "fwd.hpp"
#include "size.hpp"
#include "system_event.hpp"

namespace PanzerChasm
{

// Main game window.
// Warning, multiple windows does not work correctly!
class SystemWindow final
{
public:
	explicit SystemWindow( Settings& settings );
	~SystemWindow();

	Size2 GetViewportSize() const;

	// Commit any draw operations and show frame.
	// May wait for vsync.
	void SwapBuffers();

	void GetInput( SystemEvents& out_events );
	void CaptureMouse( bool need_capture );

private:
	Size2 viewport_size_;

	SDL_Window* window_= nullptr;
	SDL_GLContext gl_context_= nullptr;

	bool mouse_captured_= false;
};

} // namespace PanzerChasm
