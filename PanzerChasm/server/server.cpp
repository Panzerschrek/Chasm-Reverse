#include "../assert.hpp"
#include "../game_constants.hpp"
#include "../log.hpp"
#include "../math_utils.hpp"
#include "../messages_extractor.inl"
#include "../save_load_streams.hpp"
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
	, map_end_callback_( [this]{ map_end_triggered_= true; } )
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

void Server::Loop( bool paused )
{
	if( paused )
	{
		last_tick_= Time::CurrentTime();
		return;
	}

	// Accept new connections.
	while( const IConnectionPtr connection= connections_listener_->GetNewConnection() )
	{
		Log::Info( "Client \"", connection->GetConnectionInfo(), "\" connected to server" );

		players_.emplace_back( new ConnectedPlayer( connection, game_resources_, server_accumulated_time_ ) );
		ConnectedPlayer& connected_player= *players_.back();

		if( map_ != nullptr )
		{
			// Hack for game loading.
			// If flag "join_first_client_with_existing_player_" is set, and level
			// containts just one player - link with new client this player.
			bool player_joined= false;
			if( join_first_client_with_existing_player_ )
			{
				join_first_client_with_existing_player_= false;

				const Map::PlayersContainer& players= map_->GetPlayers();
				if( players.size() == 1u )
				{
					connected_player.player= players.begin()->second;
					connected_player.player_monster_id= players.begin()->first;
					player_joined= true;
				}
			}

			if( !player_joined )
				connected_player.player_monster_id=
					map_->SpawnPlayer( connected_player.player );
		}

		Messages::MapChange map_change_msg;
		map_change_msg.map_number= current_map_number_;

		connected_player.connection_info.messages_sender.SendReliableMessage( map_change_msg );

		if( map_ != nullptr )
			map_->SendMessagesForNewlyConnectedPlayer( connected_player.connection_info.messages_sender );

		Messages::PlayerSpawn spawn_message;
		connected_player.player->BuildSpawnMessage( spawn_message, true );
		spawn_message.player_monster_id= connected_player.player_monster_id;
		connected_player.connection_info.messages_sender.SendReliableMessage( spawn_message );

		connected_player.connection_info.messages_sender.Flush();
	}

	// Disconnect players.
	for( unsigned int p= 0u; p < players_.size(); )
	{
		ConnectedPlayerPtr& player= players_[p];

		if( player->connection_info.messages_extractor.IsBroken() )
		{
			Log::Info( "Messages from client \"", player->connection_info.connection->GetConnectionInfo(), "\" was broken" );
			player->connection_info.connection->Disconnect();
		}

		if( player->connection_info.connection->Disconnected() )
		{
			Log::Info( "Client \"" + player->connection_info.connection->GetConnectionInfo(), "\" disconnected from server" );

			if( map_ != nullptr )
				map_->DespawnPlayer( player->player_monster_id );

			if( p < players_.size() - 1u )
				player= std::move( players_.back() );
			players_.pop_back();
		}
		else
			p++;
	}

	// Recieve messages.
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
			ChangeMap( current_map_number_, map_ == nullptr ? Difficulty::Normal : map_->GetDifficulty(), game_rules_ );
		}
	}
}

bool Server::ChangeMap( const unsigned int map_number, const DifficultyType difficulty, const GameRules game_rules )
{
	Log::Info( "Changing server map to ", map_number );

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
		return false;
	}

	show_progress( 0.5f );
	game_rules_= game_rules;
	current_map_number_= map_number;
	current_map_data_= map_data;
	map_.reset(
		new Map(
			difficulty,
			map_data,
			game_resources_,
			server_accumulated_time_,
			map_end_callback_ ) );

	map_end_triggered_= false;
	join_first_client_with_existing_player_= false;

	for( const ConnectedPlayerPtr& connected_player : players_ )
	{
		connected_player->player->OnMapChange();

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
		connected_player->player->BuildSpawnMessage( spawn_message, true );
		spawn_message.player_monster_id= connected_player->player_monster_id;
		messages_sender.SendReliableMessage( spawn_message );

		messages_sender.Flush();
	}

	show_progress( 1.0f );

	return true;
}

void Server::Save( SaveLoadBuffer& buffer )
{
	PC_ASSERT( map_ != nullptr );
	if( map_ == nullptr ) return;

	SaveStream save_stream( buffer, server_accumulated_time_ );

	save_stream.WriteUInt32( static_cast<uint32_t>( game_rules_ ) );
	save_stream.WriteUInt32( current_map_number_ );
	save_stream.WriteUInt32( static_cast<uint32_t>( map_->GetDifficulty() ) );

	map_->Save( save_stream );
}

bool Server::Load( const SaveLoadBuffer& buffer, unsigned int& buffer_pos )
{
	DisconnectAllClients();

	Log::Info( "Loading save for server " );

	const auto show_progress=
	[&]( const float progress )
	{
		if( draw_loading_callback_ != nullptr )
			draw_loading_callback_( progress, "Server" );
	};

	show_progress( 0.0f );

	LoadStream load_stream( buffer, buffer_pos, server_accumulated_time_ );

	unsigned int game_rules;
	load_stream.ReadUInt32( game_rules );

	unsigned int map_number;
	load_stream.ReadUInt32( map_number );

	unsigned int difficulty;
	load_stream.ReadUInt32( difficulty );

	Log::Info( "Changing server map to ", map_number );

	const MapDataConstPtr map_data= map_loader_->LoadMap( map_number );
	if( map_data == nullptr )
	{
		Log::Warning( "Can not load map ", map_number );
		return false;
	}

	show_progress( 0.5f );

	game_rules_= static_cast<GameRules>( game_rules );
	current_map_number_= map_number;
	current_map_data_= map_data;
	map_.reset(
		new Map(
			static_cast<DifficultyType>(difficulty),
			map_data,
			load_stream,
			game_resources_,
			map_end_callback_ ) );

	map_end_triggered_= false;
	join_first_client_with_existing_player_= true;

	show_progress( 1.0f );

	buffer_pos= load_stream.GetBufferPos();

	return true;
}

void Server::DisconnectAllClients()
{
	if( players_.empty() )
		return;

	Log::Info( "All clients disconnected from server" );

	for( const ConnectedPlayerPtr& player : players_ )
	{
		if( map_ != nullptr )
			map_->DespawnPlayer( player->player_monster_id );
		player->connection_info.connection->Disconnect();
	}

	players_.clear();
}

void Server::operator()( const Messages::MessageBase& message )
{
	PC_ASSERT(false);
	Log::Warning( "Unknown message for server: ", int(message.message_id) );
}

void Server::operator()( const Messages::PlayerMove& message )
{
	PC_ASSERT( current_player_ != nullptr );

	if( current_player_->player->IsFullyDead() )
	{
		// Respawn when player press shoot-button.
		if( message.shoot_pressed && map_ != nullptr )
		{
			map_->DespawnPlayer( current_player_->player_monster_id );
			current_player_->player= std::make_shared<Player>( game_resources_, server_accumulated_time_ );

			if( game_rules_ == GameRules::SinglePlayer )
			{
				// Just restart map in SinglePlayer.
				ChangeMap( current_map_number_, map_->GetDifficulty(), game_rules_ );
			}
			else
			{
				// Respawn player without map restart in multiplayer modes.
				current_player_->player_monster_id= map_->SpawnPlayer( current_player_->player );

				// Send spawn message.
				Messages::PlayerSpawn spawn_message;
				current_player_->player->BuildSpawnMessage( spawn_message, true );
				spawn_message.player_monster_id= current_player_->player_monster_id;
				current_player_->connection_info.messages_sender.SendReliableMessage( spawn_message );
			}
		}
	}
	else
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
	for( const ConnectedPlayerPtr& connected_player : players_ )
		connected_player->player->GiveAmmo();

	Log::Info( "ammo added" );
}

void Server::GiveArmor()
{
	for( const ConnectedPlayerPtr& connected_player : players_ )
		connected_player->player->GiveArmor();

	Log::Info( "armor added" );
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
	god_mode_= !god_mode_;

	for( const ConnectedPlayerPtr& connected_player : players_ )
		connected_player->player->SetGodMode( god_mode_ );

	Log::Info( god_mode_ ? "chojin mode on" : "chojin mode off" );
}

void Server::ToggleNoclip()
{
	noclip_= !noclip_;

	for( const ConnectedPlayerPtr& connected_player : players_ )
		connected_player->player->SetNoclip( noclip_ );

	Log::Info( noclip_ ? "noclip on" : "noclip off" );
}

} // namespace PanzerChasm
