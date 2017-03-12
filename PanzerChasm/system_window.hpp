#pragma once
#include <vector>
#include <SDL.h>

#include "fwd.hpp"
#include "rendering_context.hpp"
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

	bool IsOpenGLRenderer() const;
	Size2 GetViewportSize() const;
	const DispaysVideoModes& GetSupportedVideoModes() const;

	RenderingContextGL GetRenderingContextGL() const;
	RenderingContextSoft GetRenderingContextSoft() const;

	void BeginFrame();
	void EndFrame();

	void SetTitle( const std::string& title );

	void GetInput( SystemEvents& out_events );
	void GetInputState( InputState& out_input_state );
	void CaptureMouse( bool need_capture );

private:
	struct PixelColorsOrder
	{
		enum Component{ R, G, B, A };
		unsigned char components_indeces[4];
	};

private:
	void GetVideoModes();

private:
	Settings& settings_;
	Size2 viewport_size_;

	SDL_Window* window_= nullptr;
	SDL_GLContext gl_context_= nullptr; // If not null - current mode is OpenGL, else - software.
	SDL_Surface* surface_= nullptr;
	PixelColorsOrder pixel_colors_order_;

	bool mouse_captured_= false;

	DispaysVideoModes displays_video_modes_;
};

} // namespace PanzerChasm
