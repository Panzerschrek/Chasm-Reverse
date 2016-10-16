#include <glsl_program.hpp>
#include <shaders_loading.hpp>

#include "log.hpp"

#include "host.hpp"

namespace PanzerChasm
{

Host::Host()
{
	vfs_= std::make_shared<Vfs>( "CSM.BIN" );

	game_resources_= std::make_shared<GameResources>();
	game_resources_->vfs= vfs_;
	LoadPalette( *vfs_, game_resources_->palette );

	system_window_.reset( new SystemWindow() );

	rSetShadersDir( "shaders" );
	{
		const auto shaders_log_callback=
		[]( const char* const log_data )
		{
			Log::Warning( log_data );
		};

		rSetShaderLoadingLogCallback( shaders_log_callback );
		r_GLSLProgram::SetProgramBuildLogOutCallback( shaders_log_callback );
	}

	menu_.reset(
		new Menu(
			system_window_->Width() ,
			system_window_->Height() ,
			game_resources_ ) );
}

Host::~Host()
{
}

bool Host::Loop()
{
	// Events processing

	if( system_window_ != nullptr )
		system_window_->GetInput( events_ );

	// TODO - send events to their destination
	for( const SystemEvent& event : events_ )
	{
		if( event.type == SystemEvent::Type::Quit )
			quit_requested_= true;
	}

	if( menu_ != nullptr )
		menu_->ProcessEvents( events_ );

	events_.clear();

	glClearColor( 0.1f, 0.1f, 0.1f, 0.5f );
	glClear( GL_COLOR_BUFFER_BIT );

	// Draw operations
	if( menu_ != nullptr )
		menu_->Draw();

	if( system_window_ )
		system_window_->SwapBuffers();

	return !quit_requested_;
}

} // namespace PanzerChasm
