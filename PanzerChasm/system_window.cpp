#include <panzer_ogl_lib.hpp>

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

SystemWindow::SystemWindow( Settings& settings )
{
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
		Log::FatalError( "Can not initialize sdl video" );

	GetVideoModes();

	int width, height;
	unsigned int frequency= 0u, display= 0u;
	if( settings.GetOrSetBool( SettingsKeys::fullscreen, false ) )
	{
		width = settings.GetInt( SettingsKeys::fullscreen_width , 640 );
		height= settings.GetInt( SettingsKeys::fullscreen_height, 480 );
		frequency= settings.GetInt( SettingsKeys::fullscreen_frequency, 60 );
		display= settings.GetInt( SettingsKeys::fullscreen_dispay, 0 );

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

		settings.SetSetting( SettingsKeys::fullscreen_dispay, int(display) );
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

	viewport_size_.Width ()= static_cast<unsigned int>( width  );
	viewport_size_.Height()= static_cast<unsigned int>( height );

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );

	window_=
		SDL_CreateWindow(
			"PanzerChasm",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			width, height,
			SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | ( fullscreen ? SDL_WINDOW_FULLSCREEN : 0 ) );

	if( window_ == nullptr )
		Log::FatalError( "Can not create window" );

	gl_context_= SDL_GL_CreateContext( window_ );
	if( gl_context_ == nullptr )
		Log::FatalError( "Can not create OpenGL context" );

	if( fullscreen )
	{
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
			settings.SetSetting( SettingsKeys::fullscreen, false );
	}

	SDL_GL_SetSwapInterval(1);

	GetGLFunctions( SDL_GL_GetProcAddress );
}

SystemWindow::~SystemWindow()
{
	SDL_GL_DeleteContext( gl_context_ );
	SDL_DestroyWindow( window_ );
	SDL_QuitSubSystem( SDL_INIT_VIDEO );
}

Size2 SystemWindow::GetViewportSize() const
{
	return viewport_size_;
}

const SystemWindow::DispaysVideoModes& SystemWindow::GetSupportedVideoModes() const
{
	return displays_video_modes_;
}

void SystemWindow::SwapBuffers()
{
	SDL_GL_SwapWindow( window_ );
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
			out_events.back().event.mouse_key.mouse_button= event.button.button;
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

void SystemWindow::GetKeyboardState( KeyboardState& out_keyboard_state )
{
	out_keyboard_state.reset();

	int key_count;
	const Uint8* const keyboard_state= SDL_GetKeyboardState( &key_count );

	for( int i= 0; i < key_count; i++ )
	{
		SystemEvent::KeyEvent::KeyCode code= TranslateKey( SDL_Scancode(i) );
		if( code < SystemEvent::KeyEvent::KeyCode::KeyCount && code != SystemEvent::KeyEvent::KeyCode::Unknown  )
			out_keyboard_state[ static_cast<unsigned int>(code) ]= keyboard_state[i] != 0;
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
