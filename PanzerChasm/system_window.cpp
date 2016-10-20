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

	case SDLK_RIGHT: return KeyCode::Right;
	case SDLK_LEFT: return KeyCode::Left;
	case SDLK_DOWN: return KeyCode::Down;
	case SDLK_UP: return KeyCode::Up;

	default:
		break;
	};

	return KeyCode::Unknown;
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

		// TODO - fill other events here

		default:
			break;
		};
	} // while events
}

} // namespace PanzerChasm
