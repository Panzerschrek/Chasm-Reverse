#include <cstring>

#include <framebuffer.hpp>
#include <glsl_program.hpp>
#include <ogl_state_manager.hpp>
#include <shaders_loading.hpp>

#include "drawers_factory_gl.hpp"
#include "drawers_factory_soft.hpp"
#include "game_resources.hpp"
#include "i_menu_drawer.hpp"
#include "i_text_drawer.hpp"
#include "log.hpp"
#include "map_loader.hpp"
#include "shared_drawers.hpp"
#include "save_load.hpp"
#include "sound/sound_engine.hpp"
#include "key_checker.hpp"
#include "shared_settings_keys.hpp"
#include "host.hpp"

namespace PanzerChasm
{

// Proxy for connections listeners.
class Host::ConnectionsListenerProxy final : public IConnectionsListener
{
public:
	ConnectionsListenerProxy(){}
	virtual ~ConnectionsListenerProxy() override {}

	void AddConnectionsListener( IConnectionsListenerPtr connections_listener )
	{
		PC_ASSERT( connections_listener != nullptr );
		connections_listeners_.emplace_back( std::move( connections_listener ) );
	}
	void ClearConnectionsListeners()
	{
		connections_listeners_.clear();
	}

public: // IConnectionsListener
	virtual IConnectionPtr GetNewConnection()
	{
		for( const IConnectionsListenerPtr& listener : connections_listeners_ )
		{
			if( const IConnectionPtr connection= listener->GetNewConnection() )
				return connection;
		}
		return nullptr;
	}

private:
	std::vector<IConnectionsListenerPtr> connections_listeners_;
};

static DifficultyType DifficultyNumberToDifficulty( const unsigned int n )
{
	switch( n )
	{
	case 0: return Difficulty::Easy;
	case 1: return Difficulty::Normal;
	case 2: return Difficulty::Hard;
	default: return Difficulty::Normal;
	};
}

Host::Host( const int argc, const char* const* const argv )
	: program_arguments_( argc, argv )
	, settings_( "PanzerChasm.cfg" )
	, commands_processor_( settings_ )
{
	{ // Register host commands
		CommandsMapPtr commands= std::make_shared<CommandsMap>();

		commands->emplace( "quit", std::bind( &Host::Quit, this ) );
		commands->emplace( "new", std::bind( &Host::NewGameCommand, this, std::placeholders::_1 ) );
		commands->emplace( "go", std::bind( &Host::RunLevelCommand, this, std::placeholders::_1 ) );
		commands->emplace( "connect", std::bind( &Host::ConnectCommand, this, std::placeholders::_1 ) );
		commands->emplace( "disconnect", std::bind( &Host::DisconnectCommand, this ) );
		commands->emplace( "runserver", std::bind( &Host::RunServerCommand, this, std::placeholders::_1 ) );
		commands->emplace( "save", std::bind( &Host::SaveCommand, this, std::placeholders::_1 ) );
		commands->emplace( "load", std::bind( &Host::LoadCommand, this, std::placeholders::_1 ) );
		commands->emplace( "vid_restart", std::bind( &Host::VidRestart, this ) );

		host_commands_= std::move( commands );
		commands_processor_.RegisterCommands( host_commands_ );
	}

	base_window_title_= "PanzerChasm";

	{
		Log::Info( "Read game archive" );

		const char* csm_file= "CSM.BIN";
		if( const char* const overrided_csm_file = program_arguments_.GetParamValue( "csm" ) )
		{
			csm_file= overrided_csm_file;
			Log::Info( "Trying to load CSM file: \"", overrided_csm_file, "\"" );
		}

		const char* const addon_path= program_arguments_.GetParamValue( "addon" );
		if( addon_path != nullptr )
		{
			base_window_title_+= " - ";
			base_window_title_+= addon_path;

			Log::Info( "Trying to load addon \"", addon_path, "\"" );
		}

		vfs_= std::make_shared<Vfs>( csm_file, addon_path );
	}

	// Create directory for saves.
	// TODO - allow user change this directory, using command-line argument or settings, for example.
	CreateSlotSavesDir();

	Log::Info( "Loading game resources" );
	game_resources_= LoadGameResources( vfs_ );

	VidRestart();

	Log::Info( "Initialize console" );
	console_.reset( new Console( commands_processor_, shared_drawers_ ) );

	if( !settings_.GetOrSetBool( "s_nosound", false ) )
	{
		Log::Info( "Initialize sound subsystem" );
		sound_engine_= std::make_shared<Sound::SoundEngine>( settings_, game_resources_ );
	}
	else
		Log::Info( "Sound disabled in settings" );

	map_loader_= std::make_shared<MapLoader>( vfs_ );

	Log::Info( "Initialize menu" );
	menu_.reset(
		new Menu(
			*this,
			shared_drawers_,
			sound_engine_ ) );

	program_arguments_.EnumerateAllParamValues(
		"exec",
		[&]( const char* const command )
		{
			commands_processor_.ProcessCommand( command );
			Loop();
		} );
}

Host::~Host()
{
}

bool Host::Loop()
{
	const Time tick_start_time= Time::CurrentTime();
	// Events processing
	InputState input_state;
	if( system_window_ != nullptr )
	{
		events_.clear();
		system_window_->GetInput( events_ );
		system_window_->GetInputState( input_state );
	}
	// Special host key events.
	for( const SystemEvent& event : events_ )
	{
		if( event.type == SystemEvent::Type::Quit )
			quit_requested_= true;

		if( event.type == SystemEvent::Type::Key && event.event.key.pressed  )
		{
			if( event.event.key.key_code == SystemEvent::KeyEvent::KeyCode::BackQuote &&
				console_ != nullptr )
				console_->Toggle();
			if( event.event.key.key_code == SystemEvent::KeyEvent::KeyCode::Pause &&
				is_single_player_ )
				paused_= !paused_;
		}
	}

	const bool input_goes_to_console= console_ != nullptr && console_->IsActive();
	const bool input_goes_to_menu= !input_goes_to_console && menu_ != nullptr && menu_->IsActive();

	const bool really_paused= paused_ || ( is_single_player_ && menu_ != nullptr && menu_->IsActive() );
	const bool playing_cutscene= client_ != nullptr && client_->PlayingCutscene();
	const bool needs_pause_server= playing_cutscene;

	system_window_->CaptureMouse( !input_goes_to_console && !input_goes_to_menu );

	if( input_goes_to_console )
		console_->ProcessEvents( events_ );

	if(
		menu_ != nullptr && !input_goes_to_console &&
		!playing_cutscene /* We must disable menu, when cutscene played. */ )
		menu_->ProcessEvents( events_ );

	if( menu_ != nullptr && !input_goes_to_menu && !input_goes_to_console && !playing_cutscene )
		menu_->ProcessEventsWhileNonactive( events_ );

	if( client_ != nullptr && !input_goes_to_console && !input_goes_to_menu && !really_paused )
		client_->ProcessEvents( events_ );

	if( sound_engine_ != nullptr )
		sound_engine_->Tick();

	// Loop operations
	if( local_server_ != nullptr )
		local_server_->Loop( really_paused || needs_pause_server );

	if( client_ != nullptr )
	{
		if( input_goes_to_console || input_goes_to_menu )
		{
			InputState dummy_input_state;
			client_->Loop( dummy_input_state, really_paused );
		}
		else
			client_->Loop( input_state, really_paused );
	}


	// Draw operations
	if( system_window_ && !system_window_->IsMinimized() )
	{
		system_window_->BeginFrame();

		if( client_ != nullptr && !client_->Disconnected() )
		{
			client_->Draw();

			if( paused_ )
				shared_drawers_->menu->DrawPaused();
		}
		else
			shared_drawers_->menu->DrawGameBackground();

		if( menu_ != nullptr )
			menu_->Draw();

		if( console_ != nullptr )
			console_->Draw();

		if( settings_.GetOrSetBool( "cl_draw_fps", false ) )
		{
			char str[32];
			const unsigned int scale= 1u;
			const unsigned int offset= shared_drawers_->menu->GetViewportSize().Width() - 4u * scale;

			std::snprintf( str, sizeof(str), "fps: %02.2f", loops_counter_.GetTicksFrequency() );
			shared_drawers_->text->Print(
				offset, 0u,
				str, scale, ITextDrawer::FontColor::Golden, ITextDrawer::Alignment::Right );

			std::snprintf( str, sizeof(str), "frame time: %03.1f ms", 1000.0f / loops_counter_.GetTicksFrequency() );
			shared_drawers_->text->Print(
				offset, shared_drawers_->text->GetLineHeight(),
				str, scale, ITextDrawer::FontColor::Golden, ITextDrawer::Alignment::Right );
		}

		system_window_->EndFrame();

		const KeyChecker key_pressed( settings_, input_state );
		if( key_pressed( SettingsKeys::key_screenshot, KeyCode::F12 ) )
			system_window_->ScreenShot();
	}


	// Try sleep just a bit, if we run too fast.
	const Time tick_end_time= Time::CurrentTime();
	const double tick_duration_ms= ( tick_end_time - tick_start_time ).ToSeconds() * 1000.0f;
	const float c_min_acceptable_tick_duration_ms= 5.0f;
	if( tick_duration_ms < 0.9f * c_min_acceptable_tick_duration_ms )
		SDL_Delay( static_cast<Uint32>( std::max( c_min_acceptable_tick_duration_ms - tick_duration_ms, 1.0 ) ) );

	loops_counter_.Tick();

	return !quit_requested_;
}

Settings& Host::GetSettings()
{
	return settings_;
}

const SystemWindow* Host::GetSystemWindow()
{
	return system_window_.get();
}

MapLoaderPtr Host::GetMapLoader()
{
	return map_loader_;
}

void Host::Quit()
{
	quit_requested_= true;
}

void Host::NewGame( const DifficultyType difficulty )
{
	DoRunLevel( 0u, difficulty );
}

void Host::ConnectToServer(
	const char* server_address,
	const uint16_t client_tcp_port,
	const uint16_t client_udp_port )
{
	InetAddress address;
	if( !InetAddress::Parse( server_address , address ) )
	{
		Log::User( "Invalid address." );
		return;
	}

	if( address.port == 0 )
		address.port= Net::c_default_server_tcp_port;

	Log::User( "Connecting to ", address.ToString(), "." );

	EnsureClient();
	EnsureNet();

	ClearBeforeGameStart();

	auto connection= net_->ConnectToServer( address, client_tcp_port, client_udp_port );
	if( connection == nullptr )
	{
		Log::User( "Connection failed." );
		return;
	}

	client_->SetConnection( connection );

	if( system_window_ != nullptr )
		system_window_->SetTitle( base_window_title_ + " - multiplayer client" );
}

void Host::StartServer(
	const unsigned int map_number,
	const DifficultyType difficulty,
	const GameRules game_rules,
	const bool dedicated,
	const uint16_t server_tcp_port,
	const uint16_t server_base_udp_port )
{
	EnsureServer();
	EnsureNet();
	if( !dedicated )
	{
		EnsureClient();
		EnsureLoopbackBuffer();
	}

	ClearBeforeGameStart();

	const IConnectionsListenerPtr listener=
		net_->CreateServerListener(
			server_tcp_port != 0u ? server_tcp_port : Net::c_default_server_tcp_port,
			server_base_udp_port != 0u ? server_base_udp_port : Net::c_default_server_udp_base_port );

	if( listener == nullptr )
	{
		Log::User( "Can not start server: network error." );
		return;
	}

	const bool map_changed=
		local_server_->ChangeMap( map_number, difficulty, game_rules );

	if( !map_changed )
		return;

	connections_listener_proxy_->AddConnectionsListener( listener );
	if( !dedicated )
		connections_listener_proxy_->AddConnectionsListener( loopback_buffer_ );

	if( !dedicated )
	{
		loopback_buffer_->RequestConnect();
		client_->SetConnection( loopback_buffer_->GetClientSideConnection() );
	}

	if( system_window_ != nullptr )
		system_window_->SetTitle( base_window_title_ + ( dedicated ? " - multiplayer dedicated server" : " - multiplayer server" ) );
}

bool Host::SaveAvailable() const
{
	return is_single_player_;
}

void Host::GetSavesNames( SavesNames& out_saves_names )
{
	//for( SaveComment& save_comment : out_saves_names )
	for( unsigned int slot= 0u; slot < c_save_slots; slot++ )
	{
		SaveComment& out_save_comment= out_saves_names[slot];

		char file_name[32];
		GetSaveFileNameForSlot( slot, file_name, sizeof(file_name) );

		if( LoadSaveComment( file_name, out_save_comment ) )
		{ /* all ok */ }
		else
			out_save_comment[0]= '\0';
	}
}

void Host::SaveGame( const unsigned int slot_number )
{
	char file_name[64];
	GetSaveFileNameForSlot( slot_number, file_name, sizeof(file_name) );
	DoSave( file_name );
}

void Host::LoadGame( const unsigned int slot_number )
{
	char file_name[64];
	GetSaveFileNameForSlot( slot_number, file_name, sizeof(file_name) );
	DoLoad( file_name );
}

void Host::VidRestart()
{
	DoVidRestart();
}

void Host::NewGameCommand( const CommandsArguments& args )
{
	DifficultyType difficulty= Difficulty::Normal;
	if( args.size() >= 1u )
		difficulty= DifficultyNumberToDifficulty( std::atoi( args[0].c_str() ) );

	NewGame( difficulty );
}

void Host::RunLevelCommand( const CommandsArguments& args )
{
	if( args.empty() )
	{
		Log::Info( "Expected map number" );
		return;
	}

	unsigned int map_number= std::atoi( args.front().c_str() );

	DifficultyType difficulty= Difficulty::Normal;
	if( args.size() >= 2u )
		difficulty= DifficultyNumberToDifficulty( std::atoi( args[1].c_str() ) );

	DoRunLevel( map_number, difficulty );
}

void Host::ConnectCommand( const CommandsArguments& args )
{
	if( args.empty() )
	{
		Log::Info( "Expected server address" );
		return;
	}

	ConnectToServer( args[0].c_str(), Net::c_default_client_tcp_port, Net::c_default_client_udp_port );
}

void Host::DisconnectCommand()
{
	ClearBeforeGameStart();
}

void Host::RunServerCommand( const CommandsArguments& args )
{
	// TODO - parse args, somehow
	PC_UNUSED(args);

	StartServer( 1u, Difficulty::Normal, GameRules::Deathmatch, false, 0u, 0u );
}

void Host::SaveCommand( const CommandsArguments& args )
{
	if( args.empty() )
	{
		Log::Warning( "Expected save name" );
		return;
	}

	DoSave( args.front().c_str() );
}

void Host::LoadCommand( const CommandsArguments& args )
{
	if( args.empty() )
	{
		Log::Warning( "Expected save name" );
		return;
	}

	DoLoad( args.front().c_str() );
}

void Host::DoVidRestart()
{
	// Clear old resources.
	if( shared_drawers_ != nullptr )
		shared_drawers_->VidClear();

	if( client_ != nullptr )
		client_->VidClear();

	// Kill old window.
	if( system_window_ != nullptr )
	{
		Log::Info( "Destroy system window" );
		system_window_= nullptr;
	}

	// Create new window and related drawers.

	Log::Info( "Create system window" );
	system_window_.reset( new SystemWindow( settings_ ) );
	system_window_->SetTitle( base_window_title_ );


	if( system_window_->IsOpenGLRenderer() )
	{
		r_OGLStateManager::ResetState();

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
		r_Framebuffer::SetScreenFramebufferSize( system_window_->GetViewportSize().Width(), system_window_->GetViewportSize().Height() );

		drawers_factory_=
			std::make_shared<DrawersFactoryGL>(
				settings_,
				game_resources_,
				system_window_->GetRenderingContextGL() );
	}
	else
	{
		// TODO - generate transformed palette in other place
		RenderingContextSoft rendering_context= system_window_->GetRenderingContextSoft();
		rendering_context.palette_transformed= std::make_shared<PaletteTransformed>();

		const Palette& in_palette= game_resources_->palette;
		for( unsigned int i= 0u; i < 256u; i++ )
		{
			unsigned char components[4];
			components[ rendering_context.color_indeces_rgba[0] ]= in_palette[ i * 3u + 0u ];
			components[ rendering_context.color_indeces_rgba[1] ]= in_palette[ i * 3u + 1u ];
			components[ rendering_context.color_indeces_rgba[2] ]= in_palette[ i * 3u + 2u ];
			components[ rendering_context.color_indeces_rgba[3] ]= i == 255u ? 0u : 255u;
			std::memcpy( &(*rendering_context.palette_transformed)[i], &components, 4u );
		}

		drawers_factory_=
			std::make_shared<DrawersFactorySoft>(
				settings_,
				game_resources_,
				rendering_context );
	}

	// If shared drawers already exist, do not create new, but reuse old structure.
	// It needs, because many subsystems( menu, client ) already have shared_ptr to it.
	if( shared_drawers_ == nullptr )
		shared_drawers_= std::make_shared<SharedDrawers>( *drawers_factory_ );
	else
		shared_drawers_->VidRestart( *drawers_factory_ );

	if( client_ != nullptr )
		client_->VidRestart( *drawers_factory_ );
}

void Host::DoRunLevel( const unsigned int map_number, const DifficultyType difficulty )
{
	EnsureClient();
	EnsureServer();
	EnsureLoopbackBuffer();

	ClearBeforeGameStart();

	const bool map_changed=
		local_server_->ChangeMap( map_number, difficulty, GameRules::SinglePlayer );
	if( !map_changed )
		return;

	// Making server listen connections from loopback buffer.
	connections_listener_proxy_->AddConnectionsListener( loopback_buffer_ );
	loopback_buffer_->RequestConnect();

	// Make client working with loopback buffer connection.
	client_->SetConnection( loopback_buffer_->GetClientSideConnection() );

	if( system_window_ != nullptr )
		system_window_->SetTitle( base_window_title_ + " - singleplayer" );

	is_single_player_= true;
}

void Host::DoSave( const char* const save_file_name )
{
	if( !( local_server_ != nullptr && client_ != nullptr && is_single_player_ ) )
	{
		Log::Warning( "Can not save now" );
		return;
	}

	Log::Info( "Save game" );

	SaveLoadBuffer buffer;
	SaveComment save_comment;

	local_server_->Save( buffer );
	client_->Save( buffer, save_comment );

	if( SaveData( save_file_name, save_comment, buffer ) )
		Log::User( "Game saved." );
	else
		Log::User( "Game save failed." );
}

void Host::DoLoad( const char* const save_file_name )
{
	SaveLoadBuffer save_buffer;
	unsigned int save_buffer_pos= 0u;

	Log::Info( "Load game" );

	if( !LoadData( save_file_name, save_buffer ) )
	{
		Log::User( "Loading failed." );
		return;
	}

	EnsureClient();
	EnsureServer();
	EnsureLoopbackBuffer();

	ClearBeforeGameStart();

	const bool map_changed=
		local_server_->Load( save_buffer, save_buffer_pos );
	if( !map_changed )
	{
		Log::User( "Loading failed." );
		return;
	}

	client_->Load( save_buffer, save_buffer_pos );

	// Making server listen connections from loopback buffer.
	connections_listener_proxy_->AddConnectionsListener( loopback_buffer_ );
	loopback_buffer_->RequestConnect();

	// Make client working with loopback buffer connection.
	client_->SetConnection( loopback_buffer_->GetClientSideConnection() );

	if( system_window_ != nullptr )
		system_window_->SetTitle( base_window_title_ + " - singleplayer" );

	is_single_player_= true;

	Log::User( "Game loaded." );
}

void Host::DrawLoadingFrame( const float progress, const char* const caption )
{
	// TODO - use this.
	PC_UNUSED( caption );

	if( system_window_ != nullptr && shared_drawers_ != nullptr )
	{
		system_window_->BeginFrame();
		shared_drawers_->menu->DrawLoading( progress );
		system_window_->EndFrame();
	}
}

MapDataConstPtr Host::CurrentMap()
{
	if( client_ == nullptr )
		return nullptr;
	return client_->CurrentMap();
}

void Host::EnsureClient()
{
	if( client_ != nullptr )
		return;

	Log::Info( "Create client" );

	const DrawLoadingCallback draw_loading_callback=
		std::bind( &Host::DrawLoadingFrame, this, std::placeholders::_1, std::placeholders::_2 );

	client_.reset(
		new Client(
			settings_,
			commands_processor_,
			game_resources_,
			map_loader_,
			*drawers_factory_,
			shared_drawers_,
			sound_engine_,
			draw_loading_callback ) );
}

void Host::EnsureServer()
{
	if( local_server_ != nullptr )
		return;

	Log::Info( "Create local server" );

	const DrawLoadingCallback draw_loading_callback=
		std::bind( &Host::DrawLoadingFrame, this, std::placeholders::_1, std::placeholders::_2 );

	PC_ASSERT( connections_listener_proxy_ == nullptr );
	connections_listener_proxy_= std::make_shared<ConnectionsListenerProxy>();

	local_server_.reset(
		new Server(
			commands_processor_,
			game_resources_,
			map_loader_,
			connections_listener_proxy_,
			draw_loading_callback ) );
}

void Host::EnsureLoopbackBuffer()
{
	if( loopback_buffer_ != nullptr )
		return;

	Log::Info( "Create loopback buffer" );
	loopback_buffer_= std::make_shared<LoopbackBuffer>();
}

void Host::EnsureNet()
{
	if( net_ != nullptr )
		return;

	Log::Info( "Initialize net subsystem" );
	net_.reset( new Net() );
}

void Host::ClearBeforeGameStart()
{
	if( system_window_ != nullptr )
		system_window_->SetTitle( base_window_title_ );

	if( client_ != nullptr )
		client_->SetConnection( nullptr );

	if( local_server_ != nullptr )
	{
		local_server_->DisconnectAllClients();
		local_server_->StopMap();
	}

	if( connections_listener_proxy_ != nullptr )
		connections_listener_proxy_->ClearConnectionsListeners();

	if( loopback_buffer_ != nullptr )
		loopback_buffer_->RequestDisconnect();

	// Force close menu before game start.
	if( menu_ != nullptr )
		menu_->Deactivate();

	is_single_player_= false;
	paused_= false;
}

} // namespace PanzerChasm
