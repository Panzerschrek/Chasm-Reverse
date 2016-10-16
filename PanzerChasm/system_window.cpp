#include <panzer_ogl_lib.hpp>

#include "log.hpp"

#include "system_window.hpp"

namespace PanzerChasm
{

SystemWindow::SystemWindow()
{
	// TODO -read from settings here
	viewport_size_[0]= 1024u;
	viewport_size_[1]= 768u;

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
			viewport_size_[0], viewport_size_[1],
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

unsigned int SystemWindow::Width () const
{
	return viewport_size_[0];
}

unsigned int SystemWindow::Height() const
{
	return viewport_size_[1];
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

		// TODO - fill other events here

		default:
			break;
		};
	} // while events
}

} // namespace PanzerChasm
