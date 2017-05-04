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
			PC_ASSERT( current_map_data_ != nullptr );

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

		if( current_map_data_ != nullptr )
		{
			Messages::MapChange map_change_msg;
			map_change_msg.need_play_cutscene= game_rules_ == GameRules::SinglePlayer && map_changed_from_previous_map_;
			map_change_msg.map_number= current_map_data_->number;
			connected_player.connection_info.messages_sender.SendReliableMessage( map_change_msg );
		}

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

	// Make several map ticks.
	for( unsigned int t= 0u; t < map_tick_count_; t++ )
	{
		// Process map inner logic
		if( map_ != nullptr )
			map_->Tick( map_ticks_[t].end, map_ticks_[t].duration );

		// Process players position
		for( const ConnectedPlayerPtr& connected_player : players_ )
		{
			if( map_ != nullptr && !connected_player->player->IsNoclip() )
				map_->ProcessPlayerPosition(
					map_ticks_[t].end,
					connected_player->player_monster_id,
					connected_player->connection_info.messages_sender );
		}
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
		connected_player->player->SendInternalMessages( messages_sender );
		messages_sender.Flush();
	}

	if( map_ != nullptr )
		map_->ClearUpdateEvents();

	// Change map, if needed at end of this loop
	if( map_end_triggered_ )
	{
		map_end_triggered_= false;

		if( map_ != nullptr &&
			current_map_data_ != nullptr &&
			current_map_data_->number < 16 )
		{
			ChangeMap(
				current_map_data_->number + 1u,
				map_ == nullptr ? Difficulty::Normal : map_->GetDifficulty(),
				game_rules_,
				true );
		}
		else
		{
			// TODO - maybe stop map here ?
		}
	}
}

bool Server::ChangeMap(
	const unsigned int map_number,
	const DifficultyType difficulty,
	const GameRules game_rules,
	const bool is_next_map_change )
{
	// Special case - process map #0 as map 1 with previous map.
	if( map_number == 0u )
		return ChangeMap( 1u, difficulty, game_rules, true );

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
	map_changed_from_previous_map_= is_next_map_change;
	current_map_data_= map_data;
	map_.reset(
		new Map(
			difficulty,
			game_rules,
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
		message.map_number= current_map_data_->number;
		message.need_play_cutscene= game_rules == GameRules::SinglePlayer && map_changed_from_previous_map_;

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

void Server::StopMap()
{
	Log::Info( "Stopping server map" );

	current_map_data_= 0u;
	map_= nullptr;
}

void Server::Save( SaveLoadBuffer& buffer )
{
	PC_ASSERT( map_ != nullptr );
	if( map_ == nullptr ) return;
	PC_ASSERT( current_map_data_ != nullptr );

	SaveStream save_stream( buffer, server_accumulated_time_ );

	save_stream.WriteUInt32( static_cast<uint32_t>( game_rules_ ) );
	save_stream.WriteUInt32( current_map_data_->number );
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
	map_changed_from_previous_map_= false;
	current_map_data_= map_data;
	map_.reset(
		new Map(
			static_cast<DifficultyType>(difficulty),
			GameRules::SinglePlayer,
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
	if( current_map_data_ == nullptr )
		return;

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
				ChangeMap( current_map_data_->number, map_->GetDifficulty(), game_rules_ );
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

	const float dt_s= dt.ToSeconds();
	const float c_min_tick_duration_s=  4.0f / 1000.0f;
	const float c_max_tick_duration_s= 30.0f / 1000.0f;
	const float c_time_eps= 2.0f / 1000.0f;

	if( dt_s < c_min_tick_duration_s )
	{
		// Skip this tick, accumulate more delta.
		map_tick_count_= 0u;
		return;
	}
	else if( dt_s > c_max_tick_duration_s )
	{
		// Devide long tick into 2 or more subticks.
		// We assume, that server runs very fast, but client on same host is very slow.
		// Therefore, make multiple fast server ticks per one slow client frame.

		map_tick_count_= static_cast<unsigned int>( std::ceil( dt_s / ( c_max_tick_duration_s - c_time_eps ) ) );
		if( map_tick_count_ > c_max_multiple_map_ticks )
		{
			// If tick count is too height, slow down game time.
			// It can happens in debugging, for example.
			map_tick_count_= c_max_multiple_map_ticks;

			const Time map_tick_dt= Time::FromSeconds( c_max_tick_duration_s );
			for( unsigned int i= 0u; i < map_tick_count_; i++ )
			{
				map_ticks_[i].end= server_accumulated_time_ + map_tick_dt * (i+1u);
				map_ticks_[i].duration= map_tick_dt;
			}
			server_accumulated_time_= map_ticks_[ map_tick_count_ - 1u ].end;
		}
		else
		{
			PC_ASSERT( map_tick_count_ >= 2u );

			const Time map_tick_dt= Time::FromSeconds( dt_s / float(map_tick_count_) );
			for( unsigned int i= 0u; i < map_tick_count_ - 1u; i++ )
			{
				map_ticks_[i].end= server_accumulated_time_ + map_tick_dt * (i+1u);
				map_ticks_[i].duration= map_tick_dt;
			}

			map_ticks_[ map_tick_count_ - 1u ].end= server_accumulated_time_ + dt;
			map_ticks_[ map_tick_count_ - 1u ].duration=
				map_ticks_[ map_tick_count_ - 1u ].end - map_ticks_[ map_tick_count_ - 2u ].end;

			server_accumulated_time_+= dt;
		}
	}
	else
	{
		// Tick time is in normal range.
		map_tick_count_= 1u;
		map_ticks_[0].end= server_accumulated_time_ + dt;
		map_ticks_[0].duration= dt;
		server_accumulated_time_+= dt;
	}

	last_tick_= current_time;
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
