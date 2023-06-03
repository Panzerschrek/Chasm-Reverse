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

	bool IsMinimized() const;
	bool IsOpenGLRenderer() const;
	Size2 GetViewportSize() const;
	const DispaysVideoModes& GetSupportedVideoModes() const;

	RenderingContextGL GetRenderingContextGL();
	RenderingContextSoft GetRenderingContextSoft();

	void BeginFrame();
	void EndFrame();

	void SetTitle( const std::string& title );

	void GetInput( SystemEvents& out_events );
	void GetInputState( InputState& out_input_state );
	void CaptureMouse( bool need_capture );
	bool ScreenShot( const std::string& file = "screenshot" ) const;

private:
	struct PixelColorsOrder
	{
		enum Component{ R, G, B, A };
		unsigned char components_indeces[4];
	};

private:
	void GetVideoModes();
	void UpdateBrightness();

	template<class ScaleGetter>
	void CopyAndScaleViewportToSystemViewport( const ScaleGetter& scale_getter );

private:
	Settings& settings_;
	Size2 viewport_size_; // Inner viewport size, not system window size.

	SDL_Window* window_= nullptr;
	SDL_Renderer* renderer_= nullptr;
	SDL_GLContext gl_context_= nullptr; // If not null - current mode is OpenGL, else - software.
	bool use_gl_context_for_software_renderer_= false;
	unsigned int software_renderer_gl_texture_= ~0u;

	// Software renderer params.
	SDL_Surface* surface_= nullptr;
	PixelColorsOrder pixel_colors_order_;

	unsigned int pixel_size_;
	std::vector<uint32_t> scaled_viewport_color_buffer_;
	unsigned int scaled_viewport_buffer_width_= 0u;

	bool mouse_captured_= false;

	float previous_brightness_= -1.0f;

	DispaysVideoModes displays_video_modes_;
};

} // namespace PanzerChasm
