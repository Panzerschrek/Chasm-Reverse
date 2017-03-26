#include <cstring>

#include <panzer_ogl_lib.hpp>

#include "assert.hpp"
#include "game_constants.hpp"
#include "log.hpp"
#include "settings.hpp"
#include "shared_settings_keys.hpp"

#include "system_window.hpp"

namespace PanzerChasm
{

static SystemEvent::KeyEvent::KeyCode TranslateKey( const SDL_Scancode scan_code )
{
	using KeyCode= SystemEvent::KeyEvent::KeyCode;

	switch( scan_code )
	{
	case SDL_SCANCODE_ESCAPE: return KeyCode::Escape;
	case SDL_SCANCODE_RETURN: return KeyCode::Enter;
	case SDL_SCANCODE_SPACE: return KeyCode::Space;
	case SDL_SCANCODE_BACKSPACE: return KeyCode::Backspace;
	case SDL_SCANCODE_GRAVE: return KeyCode::BackQuote;
	case SDL_SCANCODE_TAB: return KeyCode::Tab;

	case SDL_SCANCODE_PAGEUP: return KeyCode::PageUp;
	case SDL_SCANCODE_PAGEDOWN: return KeyCode::PageDown;

	case SDL_SCANCODE_RIGHT: return KeyCode::Right;
	case SDL_SCANCODE_LEFT: return KeyCode::Left;
	case SDL_SCANCODE_DOWN: return KeyCode::Down;
	case SDL_SCANCODE_UP: return KeyCode::Up;

	case SDL_SCANCODE_MINUS: return KeyCode::Minus;
	case SDL_SCANCODE_EQUALS: return KeyCode::Equals;

	case SDL_SCANCODE_LEFTBRACKET: return KeyCode::SquareBrackretLeft;
	case SDL_SCANCODE_RIGHTBRACKET: return KeyCode::SquareBrackretRight;

	case SDL_SCANCODE_SEMICOLON: return KeyCode::Semicolon;
	case SDL_SCANCODE_APOSTROPHE: return KeyCode::Apostrophe;
	case SDL_SCANCODE_BACKSLASH: return KeyCode::BackSlash;

	case SDL_SCANCODE_COMMA: return KeyCode::Comma;
	case SDL_SCANCODE_PERIOD: return KeyCode::Period;
	case SDL_SCANCODE_SLASH: return KeyCode::Slash;

	case SDL_SCANCODE_PAUSE: return KeyCode::Pause;

	default:
		if( scan_code >= SDL_SCANCODE_A && scan_code <= SDL_SCANCODE_Z )
			return KeyCode( int(KeyCode::A) + (scan_code - SDL_SCANCODE_A) );
		if( scan_code >= SDL_SCANCODE_1 && scan_code <= SDL_SCANCODE_9 )
			return KeyCode( int(KeyCode::K1) + (scan_code - SDL_SCANCODE_1) );
		if( scan_code == SDL_SCANCODE_0 )
			return KeyCode::K0;

	};

	return KeyCode::Unknown;
}

static SystemEvent::MouseKeyEvent::Button TranslateMouseButton( const Uint8 button )
{
	using Button= SystemEvent::MouseKeyEvent::Button;
	switch(button)
	{
	case SDL_BUTTON_LEFT: return Button::Left;
	case SDL_BUTTON_RIGHT: return Button::Right;
	case SDL_BUTTON_MIDDLE: return Button::Middle;
	};

	return Button::Unknown;
}

static SystemEvent::KeyEvent::ModifiersMask TranslateKeyModifiers( const Uint16 modifiers )
{
	SystemEvent::KeyEvent::ModifiersMask result= 0u;

	if( ( modifiers & ( KMOD_LSHIFT | KMOD_LSHIFT ) ) != 0u )
		result|= SystemEvent::KeyEvent::Modifiers::Shift;

	if( ( modifiers & ( KMOD_LCTRL | KMOD_RCTRL ) ) != 0u )
		result|= SystemEvent::KeyEvent::Modifiers::Control;

	if( ( modifiers & ( KMOD_RALT | KMOD_LALT ) ) != 0u )
		result|= SystemEvent::KeyEvent::Modifiers::Alt;

	if( ( modifiers &  KMOD_CAPS ) != 0u )
		result|= SystemEvent::KeyEvent::Modifiers::Caps;

	return result;
}

#ifdef DEBUG
static void APIENTRY GLDebugMessageCallback(
	GLenum source, GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length, const GLchar *message,
	void *userParam )
{
	(void)source;
	(void)type;
	(void)id;
	(void)severity;
	(void)length;
	(void)userParam;
	Log::Info( message );
}
#endif


template<class ScaleGetter>
void SystemWindow::CopyAndScaleViewportToSystemViewport( const ScaleGetter& scale_getter )
{
	PC_ASSERT( !IsOpenGLRenderer() );
	PC_ASSERT( pixel_size_ > 1u );

	const unsigned int scale= scale_getter();
	const unsigned int dst_width= surface_->pitch / sizeof(uint32_t);

	for( unsigned int y= 0u; y < viewport_size_.Height(); y++ )
	{
		const uint32_t* const src= scaled_viewport_color_buffer_.data() + y * scaled_viewport_buffer_width_;
		uint32_t* const dst=  static_cast<uint32_t*>(surface_->pixels) + dst_width * y * scale ;

		for( unsigned int x= 0u; x < viewport_size_.Width(); x++ )
		{
			const uint32_t color= src[x];
			for( unsigned int dy= 0u; dy < scale; dy++ )
			for( unsigned int dx= 0u; dx < scale; dx++ )
				dst[ x * scale + dx + dy * dst_width ]= color;
		}

		unsigned int pixels_left= dst_width - viewport_size_.Width() * scale;
		if( pixels_left > 0u )
		{
			unsigned int x= viewport_size_.Width();
			const uint32_t color= src[x];

			for( unsigned int dy= 0u; dy < scale; dy++ )
			for( unsigned int dx= 0u; dx < pixels_left; dx++ )
				dst[ x * scale + dx + dy * dst_width ]= color;
		}
	}

	unsigned int rows_left= surface_->h - viewport_size_.Height() * scale;
	if( rows_left > 0u )
	{
		uint32_t* const dst= reinterpret_cast<uint32_t*>( static_cast<unsigned char*>(surface_->pixels) + dst_width * viewport_size_.Height() * scale );

		for( unsigned int y= 0u; y < rows_left; y++ )
			std::memcpy(
				dst + y * dst_width,
				dst - dst_width,
				sizeof(uint32_t) * dst_width );
	}
}

SystemWindow::SystemWindow( Settings& settings )
	: settings_(settings)
{
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
		Log::FatalError( "Can not initialize sdl video" );

	GetVideoModes();

	const bool is_opengl= ! settings.GetOrSetBool( "r_software_rendering", false );

	int width= 0, height= 0, scale= 1;
	unsigned int frequency= 0u, display= 0u;
	if( settings.GetOrSetBool( SettingsKeys::fullscreen, false ) )
	{
		width = settings.GetInt( SettingsKeys::fullscreen_width , 640 );
		height= settings.GetInt( SettingsKeys::fullscreen_height, 480 );
		frequency= settings.GetInt( SettingsKeys::fullscreen_frequency, 60 );
		display= settings.GetInt( SettingsKeys::fullscreen_display, 0 );

		if( displays_video_modes_.empty() )  // Abort fullscreen.
		{
			settings.SetSetting( SettingsKeys::fullscreen, false );
			goto windowed;
		}

		if( display >= displays_video_modes_.size() )
			display= 0u;

		bool mode_found= false;
		for( const VideoMode& mode : displays_video_modes_[display] )
		{
			if( int(mode.size.Width()) == width && int(mode.size.Height()) == height )
			{
				bool frequency_found= false;
				for( const unsigned int f : mode.supported_frequencies )
				{
					if( f == frequency )
					{
						frequency_found= true;
						break;
					}
				}

				if( !frequency_found )
					frequency= mode.supported_frequencies.empty() ? 60 : mode.supported_frequencies.front();

				mode_found= true;
				break;
			}
		}

		if( !mode_found )  // Abort fullscreen.
		{
			settings.SetSetting( SettingsKeys::fullscreen, false );
			goto windowed;
		}

		settings.SetSetting( SettingsKeys::fullscreen_display, int(display) );
		settings.SetSetting( SettingsKeys::fullscreen_frequency, int(frequency) );
	}

windowed:
	const bool fullscreen= settings.GetOrSetBool( SettingsKeys::fullscreen, false );
	if( !fullscreen )
	{
		const int c_max_window_size= 4096; // TODO - get system metrics for this.

		width = settings.GetInt( SettingsKeys::window_width , 640 );
		height= settings.GetInt( SettingsKeys::window_height, 480 );
		width = std::min( std::max( int(GameConstants::min_screen_width ), width  ), c_max_window_size );
		height= std::min( std::max( int(GameConstants::min_screen_height), height ), c_max_window_size );
		settings.SetSetting( SettingsKeys::window_width , width  );
		settings.SetSetting( SettingsKeys::window_height, height );
	}

	if( !is_opengl )
	{
		scale= settings_.GetInt( "r_software_scale", 1 );
		if( scale < 1 ) scale= 1;

		// Make scale smaller, if it is very big.
		while(
			width  / scale < int(GameConstants::min_screen_width ) ||
			height / scale < int(GameConstants::min_screen_height) )
			scale--;

		settings_.SetSetting( "r_software_scale", scale );
	}
	pixel_size_= scale;

	viewport_size_.Width ()= static_cast<unsigned int>( width  / scale );
	viewport_size_.Height()= static_cast<unsigned int>( height / scale );

	if( is_opengl )
	{
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

		#ifdef DEBUG
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG );
		#endif

		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
	}

	window_=
		SDL_CreateWindow(
			"PanzerChasm",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			width, height,
			( is_opengl ? SDL_WINDOW_OPENGL : 0 ) | ( fullscreen ? SDL_WINDOW_FULLSCREEN : 0 ) | SDL_WINDOW_SHOWN );

	if( window_ == nullptr )
		Log::FatalError( "Can not create window" );

	if( is_opengl )
	{
		gl_context_= SDL_GL_CreateContext( window_ );
		if( gl_context_ == nullptr )
			Log::FatalError( "Can not create OpenGL context" );
	}

	if( fullscreen )
	{
		Log::Info( "Trying to switch to fullscreen mode ", width, "x", height, " ", frequency, "HZ on display ", display );

		bool switched= false;

		const int mode_count= SDL_GetNumDisplayModes( display );
		for( int m= 0; m < mode_count; m++ )
		{
			SDL_DisplayMode mode;
			const int result= SDL_GetDisplayMode( display, m, &mode );
			if( result < 0 )
				continue;
			if( !( SDL_BITSPERPIXEL( mode.format ) == 24 || SDL_BITSPERPIXEL( mode.format ) == 32 ) )
				continue;

			if( mode.w == int(width) && mode.h == int(height) && mode.refresh_rate == int(frequency) )
			{
				const int result= SDL_SetWindowDisplayMode( window_, &mode );
				if( result == 0 )
					switched= true;
				break;
			}
		}

		if( !switched ) // Abort fullscreen.
		{
			Log::Warning( "Can not switch to fullscreen mode, restore windowed mode" );
			settings.SetSetting( SettingsKeys::fullscreen, false );
		}
	}

	if( is_opengl )
	{
		if( settings_.GetOrSetBool( "r_gl_vsync", true ) )
			SDL_GL_SetSwapInterval(1);
		else
			SDL_GL_SetSwapInterval(0);

		GetGLFunctions( SDL_GL_GetProcAddress );

		#ifdef DEBUG
		if( glDebugMessageCallback != nullptr )
			glDebugMessageCallback( &GLDebugMessageCallback, NULL );
		#endif

		Log::Info("");
		Log::Info( "OpenGL configuration: " );
		Log::Info( "Vendor: ", glGetString( GL_VENDOR ) );
		Log::Info( "Renderer: ", glGetString( GL_RENDERER ) );
		Log::Info( "Vendor: ", glGetString( GL_VENDOR ) );
		Log::Info( "Version: ", glGetString( GL_VERSION ) );
		Log::Info("");
	}
	else
	{
		surface_= SDL_GetWindowSurface( window_ );
		if( surface_ == nullptr )
			Log::FatalError( "Can not get window surface" );

		if( surface_->format == nullptr ||
			surface_->format->BytesPerPixel != 4 )
			Log::FatalError( "Unexpected window pixel depth. Expected 32 bit" );

		const SDL_PixelFormat& pixel_format= *surface_->format;
			 if (pixel_format.Rmask ==       0xFF) pixel_colors_order_.components_indeces[ PixelColorsOrder::R ] = 0u;
		else if (pixel_format.Rmask ==     0xFF00) pixel_colors_order_.components_indeces[ PixelColorsOrder::R ] = 1u;
		else if (pixel_format.Rmask ==   0xFF0000) pixel_colors_order_.components_indeces[ PixelColorsOrder::R ] = 2u;
		else if (pixel_format.Rmask == 0xFF000000) pixel_colors_order_.components_indeces[ PixelColorsOrder::R ] = 3u;
		else pixel_colors_order_.components_indeces[ PixelColorsOrder::R ] = 255u;
			 if (pixel_format.Gmask ==       0xFF) pixel_colors_order_.components_indeces[ PixelColorsOrder::G ] = 0u;
		else if (pixel_format.Gmask ==     0xFF00) pixel_colors_order_.components_indeces[ PixelColorsOrder::G ] = 1u;
		else if (pixel_format.Gmask ==   0xFF0000) pixel_colors_order_.components_indeces[ PixelColorsOrder::G ] = 2u;
		else if (pixel_format.Gmask == 0xFF000000) pixel_colors_order_.components_indeces[ PixelColorsOrder::G ] = 3u;
		else pixel_colors_order_.components_indeces[ PixelColorsOrder::G ] = 255u;
			 if (pixel_format.Bmask ==       0xFF) pixel_colors_order_.components_indeces[ PixelColorsOrder::B ] = 0u;
		else if (pixel_format.Bmask ==     0xFF00) pixel_colors_order_.components_indeces[ PixelColorsOrder::B ] = 1u;
		else if (pixel_format.Bmask ==   0xFF0000) pixel_colors_order_.components_indeces[ PixelColorsOrder::B ] = 2u;
		else if (pixel_format.Bmask == 0xFF000000) pixel_colors_order_.components_indeces[ PixelColorsOrder::B ] = 3u;
		else pixel_colors_order_.components_indeces[ PixelColorsOrder::B ] = 255u;
			 if (pixel_format.Amask ==       0xFF) pixel_colors_order_.components_indeces[ PixelColorsOrder::A ] = 0u;
		else if (pixel_format.Amask ==     0xFF00) pixel_colors_order_.components_indeces[ PixelColorsOrder::A ] = 1u;
		else if (pixel_format.Amask ==   0xFF0000) pixel_colors_order_.components_indeces[ PixelColorsOrder::A ] = 2u;
		else if (pixel_format.Amask == 0xFF000000) pixel_colors_order_.components_indeces[ PixelColorsOrder::A ] = 3u;
		else pixel_colors_order_.components_indeces[ PixelColorsOrder::A ] = 255u;

		if( pixel_colors_order_.components_indeces[ PixelColorsOrder::R ] == 255u ||
			pixel_colors_order_.components_indeces[ PixelColorsOrder::G ] == 255u ||
			pixel_colors_order_.components_indeces[ PixelColorsOrder::B ] == 255u )
		{
			Log::Warning( "Unnknown pixels colors order" );
			pixel_colors_order_.components_indeces[ PixelColorsOrder::R ]= 0u;
			pixel_colors_order_.components_indeces[ PixelColorsOrder::R ]= 1u;
			pixel_colors_order_.components_indeces[ PixelColorsOrder::R ]= 2u;
		}

		if( pixel_size_ > 1u )
		{
			scaled_viewport_buffer_width_= ( viewport_size_.Width () + 3u ) & (~3u);
			scaled_viewport_color_buffer_.resize( scaled_viewport_buffer_width_ * viewport_size_.Height() );
		}
	}
}

SystemWindow::~SystemWindow()
{
	if( gl_context_ != nullptr )
		SDL_GL_DeleteContext( gl_context_ );

	SDL_DestroyWindow( window_ );

	SDL_QuitSubSystem( SDL_INIT_VIDEO );
}

bool SystemWindow::IsOpenGLRenderer() const
{
	return gl_context_ != nullptr;
}

Size2 SystemWindow::GetViewportSize() const
{
	return viewport_size_;
}

const SystemWindow::DispaysVideoModes& SystemWindow::GetSupportedVideoModes() const
{
	return displays_video_modes_;
}

RenderingContextGL SystemWindow::GetRenderingContextGL()
{
	PC_ASSERT( IsOpenGLRenderer() );

	RenderingContextGL result;

	result.glsl_version= r_GLSLVersion( r_GLSLVersion::v330, r_GLSLVersion::Profile::Core );
	result.viewport_size= viewport_size_;

	return result;
}

RenderingContextSoft SystemWindow::GetRenderingContextSoft()
{
	PC_ASSERT( ! IsOpenGLRenderer() );
	PC_ASSERT( surface_ != nullptr );

	RenderingContextSoft result;

	result.viewport_size= viewport_size_;
	if( pixel_size_ == 1u )
	{
		result.row_pixels= surface_->pitch / 4u;
		result.window_surface_data= static_cast<unsigned int*>( surface_->pixels );
	}
	else
	{
		result.row_pixels= scaled_viewport_buffer_width_;
		result.window_surface_data= scaled_viewport_color_buffer_.data();
	}

	result.color_indeces_rgba[0]= pixel_colors_order_.components_indeces[ PixelColorsOrder::R ];
	result.color_indeces_rgba[1]= pixel_colors_order_.components_indeces[ PixelColorsOrder::G ];
	result.color_indeces_rgba[2]= pixel_colors_order_.components_indeces[ PixelColorsOrder::B ];
	result.color_indeces_rgba[3]= pixel_colors_order_.components_indeces[ PixelColorsOrder::A ];

	return result;
}

void SystemWindow::BeginFrame()
{
	const bool need_clear= settings_.GetOrSetBool( "r_clear", false );

	if( IsOpenGLRenderer() )
	{
		glClear( ( need_clear ? GL_COLOR_BUFFER_BIT : 0 ) | GL_DEPTH_BUFFER_BIT );
	}
	else
	{
		if( pixel_size_ == 1u && SDL_MUSTLOCK( surface_ ) )
			SDL_LockSurface( surface_ );

		if( need_clear )
		{
			if( pixel_size_ == 1u )
				std::memset(
					surface_->pixels,
					0,
					surface_->pitch * surface_->h );
			else
				std::memset(
					scaled_viewport_color_buffer_.data(),
					0,
					scaled_viewport_color_buffer_.size() * sizeof(uint32_t) );
		}
	}
}

void SystemWindow::EndFrame()
{
	if( IsOpenGLRenderer() )
	{
		SDL_GL_SwapWindow( window_ );
	}
	else
	{
		if( pixel_size_ == 1u && SDL_MUSTLOCK( surface_ ) )
			SDL_UnlockSurface( surface_ );

		if( pixel_size_ > 1u )
		{
			if( SDL_MUSTLOCK( surface_ ) )
				SDL_LockSurface( surface_ );

			// Optimization.
			// Generate different functions (via template parameter) for some useful scales.
			// Compiler may optimize inner loops, if scale is constant.
			switch( pixel_size_ )
			{
			case 2u: CopyAndScaleViewportToSystemViewport( []{ return 2u; } ); break;
			case 3u: CopyAndScaleViewportToSystemViewport( []{ return 3u; } ); break;
			case 4u: CopyAndScaleViewportToSystemViewport( []{ return 4u; } ); break;
			default: CopyAndScaleViewportToSystemViewport( [this]{ return pixel_size_; } );  break;
			};

			if( SDL_MUSTLOCK( surface_ ) )
				SDL_UnlockSurface( surface_ );
		}

		SDL_UpdateWindowSurface( window_ );
	}
}

void SystemWindow::SetTitle( const std::string& title )
{
	SDL_SetWindowTitle( window_, title.c_str() );
}

void SystemWindow::GetInput( SystemEvents& out_events )
{
	out_events.clear();

	SDL_Event event;
	while( SDL_PollEvent(&event) )
	{
		switch(event.type)
		{
		case SDL_WINDOWEVENT:
			if( event.window.event == SDL_WINDOWEVENT_CLOSE )
			{
				out_events.emplace_back();
				out_events.back().type= SystemEvent::Type::Quit;
			}
			break;

		case SDL_QUIT:
			out_events.emplace_back();
			out_events.back().type= SystemEvent::Type::Quit;
			break;

		case SDL_KEYUP:
		case SDL_KEYDOWN:
			out_events.emplace_back();
			out_events.back().type= SystemEvent::Type::Key;
			out_events.back().event.key.key_code= TranslateKey( event.key.keysym.scancode );
			out_events.back().event.key.pressed= event.type == SDL_KEYUP ? false : true;
			out_events.back().event.key.modifiers= TranslateKeyModifiers( event.key.keysym.mod );
			break;

		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
			out_events.emplace_back();
			out_events.back().type= SystemEvent::Type::MouseKey;
			out_events.back().event.mouse_key.mouse_button= TranslateMouseButton( event.button.button );
			out_events.back().event.mouse_key.x= event.button.x;
			out_events.back().event.mouse_key.y= event.button.y;
			out_events.back().event.mouse_key.pressed= event.type == SDL_MOUSEBUTTONUP ? false : true;
			break;

		case SDL_MOUSEMOTION:
			out_events.emplace_back();
			out_events.back().type= SystemEvent::Type::MouseMove;
			out_events.back().event.mouse_move.dx= event.motion.xrel;
			out_events.back().event.mouse_move.dy= event.motion.yrel;
			break;

		case SDL_TEXTINPUT:
			// Accept only visible ASCII
			if( event.text.text[0] >= 32 )
			// && event.text.text[0] < 128 )
			{
				out_events.emplace_back();
				out_events.back().type= SystemEvent::Type::CharInput;
				out_events.back().event.char_input.ch= event.text.text[0];
			}
			break;

		// TODO - fill other events here

		default:
			break;
		};
	} // while events
}

void SystemWindow::GetInputState( InputState& out_input_state )
{
	out_input_state.keyboard.reset();

	int key_count;
	const Uint8* const keyboard_state= SDL_GetKeyboardState( &key_count );

	for( int i= 0; i < key_count; i++ )
	{
		SystemEvent::KeyEvent::KeyCode code= TranslateKey( SDL_Scancode(i) );
		if( code < SystemEvent::KeyEvent::KeyCode::KeyCount && code != SystemEvent::KeyEvent::KeyCode::Unknown  )
			out_input_state.keyboard[ static_cast<unsigned int>(code) ]= keyboard_state[i] != 0;
	}

	out_input_state.mouse.reset();

	const Uint32 mouse_state= SDL_GetMouseState( nullptr, nullptr );
	for( unsigned int i= SDL_BUTTON_LEFT; i <= SDL_BUTTON_RIGHT; i++ )
	{
		out_input_state.mouse[ static_cast<unsigned int>(TranslateMouseButton(i)) ]=
			( SDL_BUTTON(i) & mouse_state )!= 0u;
	}
}

void SystemWindow::CaptureMouse( const bool need_capture )
{
	if( need_capture != mouse_captured_ )
	{
		mouse_captured_= need_capture;
		SDL_SetRelativeMouseMode( need_capture ? SDL_TRUE : SDL_FALSE );
	}
}

void SystemWindow::GetVideoModes()
{
	const int display_count= SDL_GetNumVideoDisplays();
	if( display_count <= 0 )
		return;

	displays_video_modes_.reserve( static_cast<unsigned int>(display_count) );

	for( int d= 0; d < display_count; d++ )
	{
		const int display_mode_count= SDL_GetNumDisplayModes( d );
		if( display_mode_count <= 0 )
			continue;

		displays_video_modes_.emplace_back();
		VideoModes& video_modes= displays_video_modes_.back();

		for( int m= 0; m < display_mode_count; m++ )
		{
			SDL_DisplayMode mode;
			const int result= SDL_GetDisplayMode( d, m, &mode );
			if( result < 0 )
				continue;
			if( !( SDL_BITSPERPIXEL( mode.format ) == 24 || SDL_BITSPERPIXEL( mode.format ) == 32 ) ||
				mode.refresh_rate <= 0 )
				continue;

			// SDL sorts modes by width, height, bpp, freq.
			// use it and join modes with same resolution.
			const Size2 size( mode.w, mode.h );
			if( video_modes.empty() ||
				size != video_modes.back().size )
			{
				video_modes.emplace_back();
				video_modes.back().size= size;
			}

			video_modes.back().supported_frequencies.emplace_back( mode.refresh_rate );
		}
	}
}

} // namespace PanzerChasm
