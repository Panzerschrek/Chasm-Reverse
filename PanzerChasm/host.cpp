#include <glsl_program.hpp>
#include <shaders_loading.hpp>

#include "log.hpp"

#include "host.hpp"

namespace PanzerChasm
{

Host::Host()
{
	{ // Register host commands
		CommandsMapPtr commands= std::make_shared<CommandsMap>();

		commands->emplace( "quit", std::bind( &Host::Quit, this ) );
		commands->emplace( "new", std::bind( &Host::NewGame, this ) );
		commands->emplace( "go", std::bind( &Host::RunLevel, this, std::placeholders::_1 ) );

		host_commands_= std::move( commands );
		commands_processor_.RegisterCommands( host_commands_ );
	}

	Log::Info( "Read game archive" );
	vfs_= std::make_shared<Vfs>( "CSM.BIN" );

	Log::Info( "Loading game resources" );
	game_resources_= LoadGameResources( vfs_ );

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

	const DrawersPtr drawers= std::make_shared<Drawers>( rendering_context, *game_resources_ );

	Log::Info( "Initialize console" );
	console_.reset( new Console( commands_processor_, drawers ) );

	Log::Info( "Initialize menu" );
	menu_.reset(
		new Menu(
			*this,
			drawers ) );

	map_loader_= std::make_shared<MapLoader>( vfs_ );

	Log::Info( "Create loopback buffer" );
	loopback_buffer_= std::make_shared<LoopbackBuffer>();

	Log::Info( "Create local server" );
	local_server_.reset( new Server( game_resources_, map_loader_, loopback_buffer_ ) );

	loopback_buffer_->RequestConnect();
	local_server_->ChangeMap( 1 );

	Log::Info( "Create client" );
	client_.reset(
		new Client(
			game_resources_,
			map_loader_,
			loopback_buffer_,
			rendering_context,
			drawers ) );
}

Host::~Host()
{
}

bool Host::Loop()
{
	// Events processing
	if( system_window_ != nullptr )
	{
		events_.clear();
		system_window_->GetInput( events_ );
	}

	for( const SystemEvent& event : events_ )
	{
		if( event.type == SystemEvent::Type::Quit )
			quit_requested_= true;

		if( event.type == SystemEvent::Type::Key &&
			event.event.key.pressed &&
			event.event.key.key_code == SystemEvent::KeyEvent::KeyCode::BackQuote &&
			console_ != nullptr )
			console_->Toggle();
	}

	const bool input_goes_to_console= console_ != nullptr && console_->IsActive();
	const bool input_goes_to_menu= !input_goes_to_console && menu_ != nullptr && menu_->IsActive();

	if( input_goes_to_console )
		console_->ProcessEvents( events_ );

	if( menu_ != nullptr && !input_goes_to_console )
		menu_->ProcessEvents( events_ );

	if( client_ != nullptr && !input_goes_to_console && !input_goes_to_menu )
		client_->ProcessEvents( events_ );

	// Loop operations
	if( local_server_ != nullptr )
		local_server_->Loop();

	if( client_ != nullptr )
		client_->Loop();

	// Draw operations
	if( system_window_ )
	{
		// TODO - remove draww stuff from here
		glClearColor( 0.1f, 0.1f, 0.1f, 0.5f );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		if( client_ != nullptr )
			client_->Draw();

		if( menu_ != nullptr )
			menu_->Draw();

		if( console_ != nullptr )
			console_->Draw();

		system_window_->SwapBuffers();
	}

	return !quit_requested_;
}

void Host::Quit()
{
	quit_requested_= true;
}

void Host::NewGame()
{
	if( local_server_ != nullptr )
	{
		local_server_->ChangeMap(1);
	}
}

void Host::RunLevel( const CommandsArguments& args )
{
	if( args.empty() )
	{
		Log::Info( "Expected map number" );
		return;
	}

	unsigned int map_number= std::atoi( args.front().c_str() );
	if( local_server_ != nullptr )
		local_server_->ChangeMap( map_number );
}

} // namespace PanzerChasm
