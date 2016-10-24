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

	RenderingContext rendering_context;
	rendering_context.glsl_version= r_GLSLVersion( r_GLSLVersion::v330, r_GLSLVersion::Profile::Core );
	rendering_context.viewport_size= system_window_->GetViewportSize();

	menu_.reset(
		new Menu(
			*this,
			rendering_context,
			game_resources_ ) );


	map_loader_= std::make_shared<MapLoader>( vfs_ );
	loopback_buffer_= std::make_shared<LoopbackBuffer>();
	local_server_.reset( new Server( game_resources_, map_loader_, loopback_buffer_ ) );

	loopback_buffer_->RequestConnect();

	client_.reset( new Client( game_resources_, map_loader_, loopback_buffer_ ) );
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

	if( client_ != nullptr )
		client_->ProcessEvents( events_ );

	events_.clear();

	if( local_server_ != nullptr )
		local_server_->Loop();

	if( client_ != nullptr )
		client_->Loop();

	// Draw operations
	if( system_window_ )
	{
		glClearColor( 0.1f, 0.1f, 0.1f, 0.5f );
		glClear( GL_COLOR_BUFFER_BIT );

		if( menu_ != nullptr )
			menu_->Draw();
			system_window_->SwapBuffers();
	}

	return !quit_requested_;
}

void Host::Quit()
{
	quit_requested_= true;
}

} // namespace PanzerChasm
