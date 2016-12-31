#include "../assert.hpp"
#include "../game_constants.hpp"
#include "../log.hpp"
#include "../math_utils.hpp"
#include "../messages_extractor.inl"
#include "player.hpp"

#include "server.hpp"

namespace PanzerChasm
{

Server::ConnectedPlayer::ConnectedPlayer(
	const IConnectionPtr& connection,
	const GameResourcesConstPtr& game_resoruces,
	const Time current_time )
	: connection_info( connection )
	, player( std::make_shared<Player>( game_resoruces, current_time ) )
{}

Server::Server(
	CommandsProcessor& commands_processor,
	const GameResourcesConstPtr& game_resources,
	const MapLoaderPtr& map_loader,
	const IConnectionsListenerPtr& connections_listener,
	const DrawLoadingCallback& draw_loading_callback )
	: game_resources_(game_resources)
	, map_loader_(map_loader)
	, connections_listener_(connections_listener)
	, draw_loading_callback_(draw_loading_callback)
	, last_tick_( Time::CurrentTime() )
	, server_accumulated_time_( Time::FromSeconds(0) )
	, last_tick_duration_( Time::FromSeconds(0) )
{
	PC_ASSERT( game_resources_ != nullptr );
	PC_ASSERT( map_loader_ != nullptr );
	PC_ASSERT( connections_listener_ != nullptr );

	CommandsMapPtr commands= std::make_shared<CommandsMap>();

	commands->emplace( "ammo", std::bind( &Server::GiveAmmo, this ) );
	commands->emplace( "armor", std::bind( &Server::GiveArmor, this ) );
	commands->emplace( "weapon", std::bind( &Server::GiveWeapon, this ) );
	commands->emplace( "keys", std::bind( &Server::GiveKeys, this ) );
	commands->emplace( "chojin", std::bind( &Server::ToggleGodMode, this ) );
	commands->emplace( "noclip", std::bind( &Server::ToggleNoclip, this ) );

	commands_= std::move( commands );
	commands_processor.RegisterCommands( commands_ );

	UpdateTimes();
}

Server::~Server()
{}

void Server::Loop()
{
	while( const IConnectionPtr connection= connections_listener_->GetNewConnection() )
	{
		players_.emplace_back( new ConnectedPlayer( connection, game_resources_, server_accumulated_time_ ) );
		ConnectedPlayer& connected_player= *players_.back();

		if( map_ != nullptr )
		{
			connected_player.player_monster_id=
				map_->SpawnPlayer( connected_player.player );
		}

		Messages::MapChange map_change_msg;
		map_change_msg.map_number= current_map_number_;

		connected_player.connection_info.messages_sender.SendReliableMessage( map_change_msg );

		if( map_ != nullptr )
			map_->SendMessagesForNewlyConnectedPlayer( connected_player.connection_info.messages_sender );

		Messages::PlayerSpawn spawn_message;
		connected_player.player->BuildSpawnMessage( spawn_message );
		spawn_message.player_monster_id= connected_player.player_monster_id;
		connected_player.connection_info.messages_sender.SendReliableMessage( spawn_message );

		connected_player.connection_info.messages_sender.Flush();
	}

	for( const ConnectedPlayerPtr& connected_player : players_ )
	{
		current_player_= connected_player.get();
		current_player_->connection_info.messages_extractor.ProcessMessages( *this );
		current_player_= nullptr;
	}

	// Do server logic
	UpdateTimes();

	// Process map inner logic
	if( map_ != nullptr )
		map_->Tick( server_accumulated_time_, last_tick_duration_ );

	// Process players position
	for( const ConnectedPlayerPtr& connected_player : players_ )
	{
		if( map_ != nullptr && !connected_player->player->IsNoclip() )
			map_->ProcessPlayerPosition(
				server_accumulated_time_,
				connected_player->player_monster_id,
				connected_player->connection_info.messages_sender );
	}

	// Send messages
	for( const ConnectedPlayerPtr& connected_player : players_ )
	{
		MessagesSender& messages_sender= connected_player->connection_info.messages_sender;
		if( map_ != nullptr )
			map_->SendUpdateMessages( messages_sender );

		Messages::PlayerPosition position_msg;
		Messages::PlayerState state_msg;
		Messages::PlayerWeapon weapon_msg;
		Messages::PlayerSpawn spawn_msg;
		connected_player->player->BuildPositionMessage( position_msg );
		connected_player->player->BuildStateMessage( state_msg );
		connected_player->player->BuildWeaponMessage( weapon_msg );

		if( connected_player->player->BuildSpawnMessage( spawn_msg ) )
		{
			spawn_msg.player_monster_id= connected_player->player_monster_id;
			messages_sender.SendUnreliableMessage( spawn_msg );
		}

		messages_sender.SendUnreliableMessage( position_msg );
		messages_sender.SendUnreliableMessage( state_msg );
		messages_sender.SendUnreliableMessage( weapon_msg );
		messages_sender.Flush();
	}

	if( map_ != nullptr )
		map_->ClearUpdateEvents();

	// Change map, if needed at end of this loop
	if( map_end_triggered_ )
	{
		map_end_triggered_= false;

		if( map_ != nullptr &&
			current_map_number_ < 16u )
		{
			current_map_number_++;
			ChangeMap( current_map_number_, map_ == nullptr ? Difficulty::Normal : map_->GetDifficulty() );
		}
	}
}

void Server::ChangeMap( const unsigned int map_number, const DifficultyType difficulty )
{
	const auto show_progress=
	[&]( const float progress )
	{
		if( draw_loading_callback_ != nullptr )
			draw_loading_callback_( progress, "Server" );
	};

	show_progress( 0.0f );
	const MapDataConstPtr map_data= map_loader_->LoadMap( map_number );

	if( map_data == nullptr )
	{
		Log::Warning( "Can not load map ", map_number );
		return;
	}

	show_progress( 0.5f );
	current_map_number_= map_number;
	current_map_data_= map_data;
	map_.reset(
		new Map(
			difficulty,
			map_data,
			game_resources_,
			server_accumulated_time_,
			[this](){ map_end_triggered_= true; } ) );

	map_end_triggered_= false;

	state_= State::PlayingMap;

	for( const ConnectedPlayerPtr& connected_player : players_ )
	{
		connected_player->player_monster_id=
			map_->SpawnPlayer( connected_player->player );
	}

	show_progress( 0.666f );
	for( const ConnectedPlayerPtr& connected_player : players_ )
	{
		Messages::MapChange message;
		message.map_number= current_map_number_;

		MessagesSender& messages_sender= connected_player->connection_info.messages_sender;

		messages_sender.SendReliableMessage( message );
		map_->SendMessagesForNewlyConnectedPlayer( messages_sender );

		Messages::PlayerSpawn spawn_message;
		connected_player->player->BuildSpawnMessage( spawn_message );
		spawn_message.player_monster_id= connected_player->player_monster_id;
		messages_sender.SendReliableMessage( spawn_message );

		messages_sender.Flush();
	}

	show_progress( 1.0f );
}

void Server::operator()( const Messages::MessageBase& message )
{
	PC_ASSERT(false);
	Log::Warning( "Unknown message for server: ", int(message.message_id) );
}

void Server::operator()( const Messages::PlayerMove& message )
{
	PC_ASSERT( current_player_ != nullptr );
	current_player_->player->UpdateMovement( message );
}

void Server::UpdateTimes()
{
	const Time current_time= Time::CurrentTime();
	Time dt= current_time - last_tick_;
	dt= std::min( dt, Time::FromSeconds( 0.060 ) );
	dt= std::max( dt, Time::FromSeconds( 0.002 ) );
	last_tick_duration_= dt;

	last_tick_= current_time;

	server_accumulated_time_+= last_tick_duration_;
}

void Server::GiveAmmo()
{
}

void Server::GiveArmor()
{
}

void Server::GiveWeapon()
{
	for( const ConnectedPlayerPtr& connected_player : players_ )
		connected_player->player->GiveWeapon();

	Log::Info( "weapons added" );
}

void Server::GiveKeys()
{
	for( const ConnectedPlayerPtr& connected_player : players_ )
		connected_player->player->GiveAllKeys();

	Log::Info( "give all keys" );
}

void Server::ToggleGodMode()
{
}

void Server::ToggleNoclip()
{
	noclip_= !noclip_;

	for( const ConnectedPlayerPtr& connected_player : players_ )
		connected_player->player->SetNoclip( noclip_ );

	Log::Info( noclip_ ? "noclip on" : "noclip off" );
}

} // namespace PanzerChasm
