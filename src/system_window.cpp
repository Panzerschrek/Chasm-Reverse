#include <cmath>
#include <cstring>

#include <panzer_ogl_lib.hpp>

#include "assert.hpp"
#include "game_constants.hpp"
#include "log.hpp"
#include "settings.hpp"
#include "shared_settings_keys.hpp"

#include "system_window.hpp"
#include "common/tga.hpp"
#include "common/files.hpp"

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

	case SDL_SCANCODE_LEFTBRACKET: return KeyCode::SquareBracketLeft;
	case SDL_SCANCODE_RIGHTBRACKET: return KeyCode::SquareBracketRight;

	case SDL_SCANCODE_SEMICOLON: return KeyCode::Semicolon;
	case SDL_SCANCODE_APOSTROPHE: return KeyCode::Apostrophe;
	case SDL_SCANCODE_BACKSLASH: return KeyCode::BackSlash;
	case SDL_SCANCODE_CAPSLOCK: return KeyCode::CapsLock;
	case SDL_SCANCODE_LCTRL: return KeyCode::LeftControl;
	case SDL_SCANCODE_RCTRL: return KeyCode::RightControl;
	case SDL_SCANCODE_LALT: return KeyCode::LeftAlt;
	case SDL_SCANCODE_RALT: return KeyCode::RightAlt;
	case SDL_SCANCODE_LSHIFT: return KeyCode::LeftShift;
	case SDL_SCANCODE_RSHIFT: return KeyCode::RightShift;
	case SDL_SCANCODE_COMMA: return KeyCode::Comma;
	case SDL_SCANCODE_PERIOD: return KeyCode::Period;
	case SDL_SCANCODE_SLASH: return KeyCode::Slash;
	case SDL_SCANCODE_LGUI: return KeyCode::LeftMetaGUI;
	case SDL_SCANCODE_RGUI: return KeyCode::RightMetaGUI;
	case SDL_SCANCODE_APPLICATION: return KeyCode::Application;
	case SDL_SCANCODE_PAUSE: return KeyCode::Pause;
	case SDL_SCANCODE_KP_DIVIDE: return KeyCode::KPDivide;
	case SDL_SCANCODE_KP_MULTIPLY: return KeyCode::KPMultiply;
	case SDL_SCANCODE_KP_MINUS: return KeyCode::KPMinus;
	case SDL_SCANCODE_KP_PLUS: return KeyCode::KPPlus;
	case SDL_SCANCODE_KP_ENTER: return KeyCode::KPEnter;
	case SDL_SCANCODE_KP_1: return KeyCode::KP1;
	case SDL_SCANCODE_KP_2: return KeyCode::KP2;
	case SDL_SCANCODE_KP_3: return KeyCode::KP3;
	case SDL_SCANCODE_KP_4: return KeyCode::KP4;
	case SDL_SCANCODE_KP_5: return KeyCode::KP5;
	case SDL_SCANCODE_KP_6: return KeyCode::KP6;
	case SDL_SCANCODE_KP_7: return KeyCode::KP7;
	case SDL_SCANCODE_KP_8: return KeyCode::KP8;
	case SDL_SCANCODE_KP_9: return KeyCode::KP9;
	case SDL_SCANCODE_KP_0: return KeyCode::KP0;
	case SDL_SCANCODE_KP_PERIOD: return KeyCode::KPPeriod;
	default:
		if( scan_code >= SDL_SCANCODE_A && scan_code <= SDL_SCANCODE_Z )
			return KeyCode( KeyCode::A + (scan_code - SDL_SCANCODE_A) );
		else if( scan_code >= SDL_SCANCODE_1 && scan_code <= SDL_SCANCODE_0 )
			return KeyCode( KeyCode::K1 + (scan_code - SDL_SCANCODE_1) );
		else if( scan_code >= SDL_SCANCODE_F1 && scan_code <= SDL_SCANCODE_F12 )
			return KeyCode( int(KeyCode::F1) + (scan_code - SDL_SCANCODE_F1) );
		break;
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
	GLsizei length, const GLchar* message,
	const void* userParam )
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

	const bool is_opengl= ! settings.GetOrSetBool( SettingsKeys::software_rendering, true );

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
		scale= settings_.GetInt( SettingsKeys::software_scale, 1 );
		if( scale < 1 ) scale= 1;

		// Make scale smaller, if it is very big.
		while(
			width  / scale < int(GameConstants::min_screen_width ) ||
			height / scale < int(GameConstants::min_screen_height) )
			scale--;

		settings_.SetSetting( SettingsKeys::software_scale, scale );
	}
	pixel_size_= scale;

	viewport_size_.Width ()= static_cast<unsigned int>( width  / scale );
	viewport_size_.Height()= static_cast<unsigned int>( height / scale );

	use_gl_context_for_software_renderer_= !is_opengl && settings_.GetOrSetBool( "r_software_use_gl_screen_update", true );
	if( is_opengl )
	{
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );

		#ifdef DEBUG
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG );
		#endif

		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

		// 0 - no msaa, 1 - msaa2x, 2 - msaa4x, 3 - msaa8x, 4 - msaa16x
		const unsigned int msaa_level= std::max( 0, std::min( 4, settings_.GetInt( SettingsKeys::opengl_msaa_level, 2 ) ) );
		settings_.SetSetting( SettingsKeys::opengl_msaa_level, int(msaa_level) );
		if( msaa_level > 0 )
		{
			const unsigned int msaa_samples= 1u << msaa_level;
			SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
			SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, msaa_samples );
		}
	}
	else
	{
		// In this mode we required most simple opengl - 1.1 version. It does`nt requires any flags.
	}
	SDL_ClearError();
	if(SDL_CreateWindowAndRenderer(width, height,( (is_opengl || use_gl_context_for_software_renderer_) ? SDL_WINDOW_OPENGL : 0 ) | ( fullscreen ? SDL_WINDOW_FULLSCREEN : 0 ) | SDL_WINDOW_SHOWN, &window_, &renderer_) != 0)
	{
		SDL_Log("Can not create window: %s\n", SDL_GetError());
	}

	if( window_ == nullptr || renderer_ == nullptr )
	{
		Log::FatalError( "Can not create window" );
	}

	if( is_opengl || use_gl_context_for_software_renderer_ )
	{
		gl_context_= SDL_GL_CreateContext( window_ );
		if( gl_context_ == nullptr )
			Log::FatalError( "Can not create OpenGL context" );

		Log::Info("");
		Log::Info( "OpenGL configuration: " );
		Log::Info( "Vendor: ", glGetString( GL_VENDOR ) );
		Log::Info( "Renderer: ", glGetString( GL_RENDERER ) );
		Log::Info( "Version: ", glGetString( GL_VERSION ) );
		Log::Info("");

		if( settings_.GetOrSetBool( "r_gl_vsync", true ) )
			SDL_GL_SetSwapInterval(1);
		else
			SDL_GL_SetSwapInterval(0);
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
		GetGLFunctions( SDL_GL_GetProcAddress );

		#ifdef DEBUG
		// Do reinterpret_cast, because on different platforms arguments of GLDEBUGPROC have
		// different const qualifier. Just ignore this cualifiers - we always have 'const'.
		if( glDebugMessageCallback != nullptr )
			glDebugMessageCallback( reinterpret_cast<GLDEBUGPROC>(&GLDebugMessageCallback), nullptr );
		#endif
	}
	else if( use_gl_context_for_software_renderer_ )
	{
		pixel_colors_order_.components_indeces[ PixelColorsOrder::R ]= 0u;
		pixel_colors_order_.components_indeces[ PixelColorsOrder::G ]= 1u;
		pixel_colors_order_.components_indeces[ PixelColorsOrder::B ]= 2u;
		pixel_colors_order_.components_indeces[ PixelColorsOrder::A ]= 3u;

		scaled_viewport_buffer_width_= viewport_size_.Width();
		scaled_viewport_color_buffer_.resize( scaled_viewport_buffer_width_ * viewport_size_.Height() );

		glGenTextures( 1, &software_renderer_gl_texture_ );
		glBindTexture( GL_TEXTURE_2D, software_renderer_gl_texture_ );
		glTexImage2D(
			GL_TEXTURE_2D, 0,
			GL_RGBA8, viewport_size_.Width(), viewport_size_.Height(),
			0,
			GL_RGBA, GL_UNSIGNED_BYTE, nullptr );
		const GLint filtration=
			( pixel_size_ > 1u && settings_.GetOrSetBool( "r_software_gl_update_smooth", true ) )
				? GL_LINEAR : GL_NEAREST;
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtration );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtration );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
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
			pixel_colors_order_.components_indeces[ PixelColorsOrder::G ]= 1u;
			pixel_colors_order_.components_indeces[ PixelColorsOrder::B ]= 2u;
			pixel_colors_order_.components_indeces[ PixelColorsOrder::A ]= 2u;
		}

		if( pixel_colors_order_.components_indeces[ PixelColorsOrder::A ] == 255u )
			pixel_colors_order_.components_indeces[ PixelColorsOrder::A ]=
				6u -
				pixel_colors_order_.components_indeces[ PixelColorsOrder::R ] -
				pixel_colors_order_.components_indeces[ PixelColorsOrder::G ] -
				pixel_colors_order_.components_indeces[ PixelColorsOrder::B ];

		if( pixel_size_ > 1u )
		{
			scaled_viewport_buffer_width_= ( viewport_size_.Width () + 3u ) & (~3u);
			scaled_viewport_color_buffer_.resize( scaled_viewport_buffer_width_ * viewport_size_.Height() );
		}
	}
}

SystemWindow::~SystemWindow()
{
	if( software_renderer_gl_texture_ != ~0u )
		glDeleteTextures( 1u, &software_renderer_gl_texture_ );

	if( gl_context_ != nullptr )
		SDL_GL_DeleteContext( gl_context_ );

	SDL_DestroyWindow( window_ );

	SDL_QuitSubSystem( SDL_INIT_VIDEO );
}

bool SystemWindow::IsMinimized() const
{
	return ( SDL_GetWindowFlags( window_ ) & SDL_WINDOW_MINIMIZED ) != 0;
}

bool SystemWindow::IsOpenGLRenderer() const
{
	return gl_context_ != nullptr && !use_gl_context_for_software_renderer_;
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
	PC_ASSERT( surface_ != nullptr || use_gl_context_for_software_renderer_ );

	RenderingContextSoft result;

	result.viewport_size= viewport_size_;
	if( pixel_size_ == 1u && !use_gl_context_for_software_renderer_ )
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
	UpdateBrightness();

	const bool need_clear= settings_.GetOrSetBool( "r_clear", false );

	if( IsOpenGLRenderer() )
	{
		if( need_clear )
			glClear( GL_COLOR_BUFFER_BIT );
	}
	else if( use_gl_context_for_software_renderer_ )
	{
		if( need_clear )
			std::memset(
				scaled_viewport_color_buffer_.data(),
				0,
				scaled_viewport_color_buffer_.size() * sizeof(uint32_t) );
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
	else if( use_gl_context_for_software_renderer_ )
	{
		glBindTexture( GL_TEXTURE_2D, software_renderer_gl_texture_ );
		glTexSubImage2D(
			GL_TEXTURE_2D, 0,
			0, 0, viewport_size_.Width(), viewport_size_.Height(),
			GL_RGBA, GL_UNSIGNED_BYTE, scaled_viewport_color_buffer_.data() );

		glEnable( GL_TEXTURE_2D );

		glBegin( GL_QUADS );
		glTexCoord2f( 0.0f, 0.0f ); glVertex2f( -1.0f, -1.0f * -1.0f );
		glTexCoord2f( 1.0f, 0.0f ); glVertex2f( +1.0f, -1.0f * -1.0f );
		glTexCoord2f( 1.0f, 1.0f ); glVertex2f( +1.0f, +1.0f * -1.0f );
		glTexCoord2f( 0.0f, 1.0f ); glVertex2f( -1.0f, +1.0f * -1.0f );
		glEnd();

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
		case SDL_MOUSEWHEEL:
			out_events.emplace_back();
			out_events.back().type= SystemEvent::Type::Wheel;
#ifdef SDL_MOUSEWHEEL_FLIPPED
			out_events.back().event.wheel.delta = (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) ? -event.wheel.y : event.wheel.y;
#else
			out_events.back().event.wheel.delta = event.wheel.y;
#endif
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

void SystemWindow::UpdateBrightness()
{
	float new_brightness= settings_.GetFloat( SettingsKeys::brightness, 0.5f );
	if( new_brightness < 0.0f ) new_brightness= 0.0f;
	if( new_brightness > 1.0f ) new_brightness= 1.0f;
	settings_.SetSetting( SettingsKeys::brightness, new_brightness );

	if( previous_brightness_ <= 0.0f || previous_brightness_ != new_brightness )
	{
		previous_brightness_= new_brightness;

		Uint16 gamma_ramp[256];

		const float c_min_gamma= 0.4f;
		const float c_max_gamma= 1.5f;
		const float gamma= c_max_gamma - new_brightness * ( c_max_gamma - c_min_gamma );
		for( unsigned int i= 0u; i < 256u; i++ )
		{
			float p= std::pow( ( float(i) + 0.5f ) / 255.5f, gamma ) * 65535.0f;
			int p_i= int(std::round(p));
			if( p_i < 0 ) p_i= 0;
			if( p_i > 65535 ) p_i= 65535;
			gamma_ramp[i]= p_i;
		}
		SDL_SetWindowGammaRamp( window_, gamma_ramp, gamma_ramp, gamma_ramp );
	}
}

bool SystemWindow::ScreenShot( const std::string& file ) const
{
	int result = -1;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	std::array<Uint32, 4> mask = { 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff };
#else
	std::array<Uint32, 4> mask = { 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 };
#endif
	/* get screen surface from window */
	SDL_Surface* screen = SDL_GetWindowSurface(window_);
	int size = screen->w * screen->h;
	std::vector<std::array<uint8_t, 4>> pixels(size);

	/* get opengl settings */
	const bool is_opengl= !settings_.GetOrSetBool( SettingsKeys::software_rendering, true );

	if (!pixels.empty())
	{
		if( is_opengl || use_gl_context_for_software_renderer_ )
		{
			/* set pack alignment */
			GLint pack_aligment;
			glGetIntegerv(GL_PACK_ALIGNMENT, &pack_aligment);
			glPixelStorei(GL_PACK_ALIGNMENT, 4);
			glFlush();

			glReadPixels(0, 0, screen->w, screen->h, GL_RGBA, GL_UNSIGNED_BYTE, &pixels.front());

			glPixelStorei(GL_PACK_ALIGNMENT, pack_aligment);

		}
		else
		{
			SDL_Rect* viewport = nullptr;
			SDL_GetClipRect(screen, viewport);
			SDL_LockSurface(screen);
			SDL_ClearError();
			if((result = SDL_RenderReadPixels(renderer_, viewport, SDL_PIXELFORMAT_RGBA32, &pixels.front(), pixels.size() * sizeof(std::array<uint8_t, 4>))) != 0);
			{
				SDL_Log("Couldn't read screen: %s\n", SDL_GetError());
				pixels.clear();
			}
			SDL_UnlockSurface(screen);
		}

		if(!pixels.empty())
		{
			/* apply gamma ramp */
			std::array<uint16_t, 256> r, g, b;
			SDL_GetWindowGammaRamp(window_, r.begin(), g.begin(), b.begin());
			for (auto & p : pixels)
				p = { (uint8_t)(r[p[0]] >> 8), (uint8_t)(g[p[1]] >> 8), (uint8_t)(b[p[2]] >> 8), p[3] };

			/* create target directory */
			if(!create_directory(dir_name<std::string>(file)))
				Log::Warning("Couldn't create screenshot directory: ", strerror(errno));

			ChasmReverse::WriteTGA(screen->w, screen->h, &pixels.front().front(), nullptr, (remove_extension<std::string>(file) + ".tga").c_str());

			pixels.clear();
		}

	}
	SDL_ClearError();
	SDL_FreeSurface(screen);
	return result == 0 ? true : false;
}

}// namespace PanzerChasm
