#include <panzer_ogl_lib.hpp>

#include "log.hpp"

#include "system_window.hpp"

namespace PanzerChasm
{

static SystemEvent::KeyEvent::KeyCode TranslateKey( const SDL_Keycode key_code )
{
	using KeyCode= SystemEvent::KeyEvent::KeyCode;

	switch( key_code )
	{
	case SDLK_ESCAPE: return KeyCode::Escape;
	case SDLK_RETURN: return KeyCode::Enter;
	case SDLK_SPACE: return KeyCode::Space;
	case SDLK_BACKSPACE: return KeyCode::Backspace;
	case SDLK_BACKQUOTE: return KeyCode::BackQuote;

	case SDLK_PAGEUP: return KeyCode::PageUp;
	case SDLK_PAGEDOWN: return KeyCode::PageDown;

	case SDLK_RIGHT: return KeyCode::Right;
	case SDLK_LEFT: return KeyCode::Left;
	case SDLK_DOWN: return KeyCode::Down;
	case SDLK_UP: return KeyCode::Up;

	default:
		if( key_code >= SDLK_a && key_code <= SDLK_z )
			return KeyCode( int(KeyCode::A) + (key_code - SDLK_a) );
		if( key_code >= SDLK_0 && key_code <= SDLK_9 )
			return KeyCode( int(KeyCode::K0) + (key_code - SDLK_0) );

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

SystemWindow::SystemWindow()
{
	// TODO -read from settings here
	viewport_size_.Width ()= 1024u;
	viewport_size_.Height()=  768u;

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
			viewport_size_.Width(), viewport_size_.Height(),
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
		case SDL_QUIT:
			out_events.emplace_back();
			out_events.back().type= SystemEvent::Type::Quit;
			break;

		case SDL_KEYUP:
		case SDL_KEYDOWN:
			out_events.emplace_back();
			out_events.back().type= SystemEvent::Type::Key;
			out_events.back().event.key.key_code= TranslateKey( event.key.keysym.sym );
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

} // namespace PanzerChasm
