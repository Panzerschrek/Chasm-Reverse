#pragma once
#include <vector>
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
	struct VideoMode
	{
		Size2 size;
		std::vector<unsigned int> supported_frequencies;
	};

	typedef std::vector<VideoMode> VideoModes;
	typedef std::vector<VideoModes> DispaysVideoModes;

public:
	explicit SystemWindow( Settings& settings );
	~SystemWindow();

	Size2 GetViewportSize() const;
	const DispaysVideoModes& GetSupportedVideoModes() const;

	// Commit any draw operations and show frame.
	// May wait for vsync.
	void SwapBuffers();

	void GetInput( SystemEvents& out_events );
	void GetKeyboardState( KeyboardState& out_keyboard_state );
	void CaptureMouse( bool need_capture );

private:
	void GetVideoModes();

private:
	Size2 viewport_size_;

	SDL_Window* window_= nullptr;
	SDL_GLContext gl_context_= nullptr;

	bool mouse_captured_= false;

	DispaysVideoModes displays_video_modes_;
};

} // namespace PanzerChasm
