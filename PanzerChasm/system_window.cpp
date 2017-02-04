#include <panzer_ogl_lib.hpp>

#include "game_constants.hpp"
#include "log.hpp"
#include "settings.hpp"

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
	static const char c_width_key []= "r_window_width" ;
	static const char c_height_key[]= "r_window_height";
	const int c_max_window_size= 4096; // TODO - get system metrics for this.

	int width = settings.GetInt( c_width_key , 640 );
	int height= settings.GetInt( c_height_key, 480 );
	width = std::min( std::max( int(GameConstants::min_screen_width ), width  ), c_max_window_size );
	height= std::min( std::max( int(GameConstants::min_screen_height), height ), c_max_window_size );
	settings.SetSetting( c_width_key , width  );
	settings.SetSetting( c_height_key, height );

	viewport_size_.Width ()= static_cast<unsigned int>( width  );
	viewport_size_.Height()= static_cast<unsigned int>( height );

	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
		Log::FatalError( "Can not initialize sdl video" );

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
			SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );

	if( window_ == nullptr )
		Log::FatalError( "Can not create window" );

	gl_context_= SDL_GL_CreateContext( window_ );
	if( gl_context_ == nullptr )
		Log::FatalError( "Can not create OpenGL context" );

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

void SystemWindow::CaptureMouse( const bool need_capture )
{
	if( need_capture != mouse_captured_ )
	{
		mouse_captured_= need_capture;
		SDL_SetRelativeMouseMode( need_capture ? SDL_TRUE : SDL_FALSE );
	}
}

} // namespace PanzerChasm
