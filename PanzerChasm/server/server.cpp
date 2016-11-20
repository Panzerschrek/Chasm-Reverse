#include <matrix.hpp>

#include "../assert.hpp"
#include "../game_constants.hpp"
#include "../log.hpp"
#include "../math_utils.hpp"
#include "../messages_extractor.inl"

#include "server.hpp"

namespace PanzerChasm
{

Server::ConnectedPlayer::ConnectedPlayer( const IConnectionPtr& connection )
	: connection_info( connection )
	, player()
{}

Server::Server(
	CommandsProcessor& commands_processor,
	const GameResourcesConstPtr& game_resources,
	const MapLoaderPtr& map_loader,
	const IConnectionsListenerPtr& connections_listener )
	: game_resources_(game_resources)
	, map_loader_(map_loader)
	, connections_listener_(connections_listener)
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
}

Server::~Server()
{}

void Server::Loop()
{
	while( const IConnectionPtr connection= connections_listener_->GetNewConnection() )
	{
		players_.emplace_back( new ConnectedPlayer( connection ) );
		ConnectedPlayer& connected_player= *players_.back();

		if( map_ != nullptr )
			map_->SpawnPlayer( connected_player.player );

		Messages::MapChange map_change_msg;
		map_change_msg.message_id= MessageId::MapChange;
		map_change_msg.map_number= current_map_number_;

		connected_player.connection_info.messages_sender.SendReliableMessage( map_change_msg );

		if( map_ != nullptr )
			map_->SendMessagesForNewlyConnectedPlayer( connected_player.connection_info.messages_sender );

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

	// Move players
	for( const ConnectedPlayerPtr& connected_player : players_ )
	{
		connected_player->player.Move( last_tick_duration_ );
	}

	// Process players position
	for( const ConnectedPlayerPtr& connected_player : players_ )
	{
		if( map_ != nullptr && !connected_player->player.IsNoclip() )
			map_->ProcessPlayerPosition(
				server_accumulated_time_,
				connected_player->player,
				connected_player->connection_info.messages_sender );

	}

	// Process map inner logic
	if( map_ != nullptr )
		map_->Tick( server_accumulated_time_ );

	// Send messages
	for( const ConnectedPlayerPtr& connected_player : players_ )
	{
		MessagesSender& messages_sender= connected_player->connection_info.messages_sender;
		if( map_ != nullptr )
			map_->SendUpdateMessages( messages_sender );

		Messages::PlayerPosition position_msg;
		position_msg.message_id= MessageId::PlayerPosition;

		for( unsigned int j= 0u; j < 3u; j++ )
			position_msg.xyz[j]= static_cast<short>( connected_player->player.Position().ToArr()[j] * 256.0f );

		Messages::PlayerState state_msg;
		connected_player->player.BuildStateMessage( state_msg );

		messages_sender.SendUnreliableMessage( position_msg );
		messages_sender.SendUnreliableMessage( state_msg );
		messages_sender.Flush();
	}

	if( map_ != nullptr )
		map_->ClearUpdateEvents();
}

void Server::ChangeMap( const unsigned int map_number )
{
	const MapDataConstPtr map_data= map_loader_->LoadMap( map_number );

	if( map_data == nullptr )
	{
		Log::Warning( "Can not load map ", map_number );
		return;
	}

	current_map_number_= map_number;
	current_map_data_= map_data;
	map_.reset( new Map( map_data, game_resources_, last_tick_ ) );

	state_= State::PlayingMap;

	for( const ConnectedPlayerPtr& connected_player : players_ )
		map_->SpawnPlayer( connected_player->player );

	for( const ConnectedPlayerPtr& connected_player : players_ )
	{
		Messages::MapChange message;
		message.message_id= MessageId::MapChange;
		message.map_number= current_map_number_;

		MessagesSender& messages_sender= connected_player->connection_info.messages_sender;

		messages_sender.SendReliableMessage( message );
		map_->SendMessagesForNewlyConnectedPlayer( messages_sender );
		messages_sender.Flush();
	}
}

void Server::operator()( const Messages::MessageBase& message )
{
	PC_ASSERT(false);
	Log::Warning( "Unknown message for server: ", int(message.message_id) );
}

void Server::operator()( const Messages::PlayerMove& message )
{
	PC_ASSERT( current_player_ != nullptr );
	current_player_->player.UpdateMovement( message );
}

void Server::operator()( const Messages::PlayerShot& message )
{
	// TODO - clear this
	PC_ASSERT( current_player_ != nullptr );

	const m_Vec3 view_vec( 0.0f, 1.0f, 0.0f );

	m_Mat4 x_rotate, z_rotate;

	x_rotate.RotateX( MessageAngleToAngle( message.view_dir_angle_x ) );
	z_rotate.RotateZ( MessageAngleToAngle( message.view_dir_angle_z ) );

	const m_Vec3 view_vec_rotated= view_vec * x_rotate * z_rotate;

	map_->Shoot(
		0u,
		current_player_->player.Position() + m_Vec3( 0.0f, 0.0f, GameConstants::player_eyes_level ),
		view_vec_rotated,
		server_accumulated_time_ );
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
}

void Server::GiveKeys()
{
	for( const ConnectedPlayerPtr& connected_player : players_ )
		connected_player->player.GiveAllKeys();

	Log::Info( "give all keys" );
}

void Server::ToggleGodMode()
{
}

void Server::ToggleNoclip()
{
	noclip_= !noclip_;

	for( const ConnectedPlayerPtr& connected_player : players_ )
		connected_player->player.SetNoclip( noclip_ );

	Log::Info( noclip_ ? "noclip on" : "noclip off" );
}

} // namespace PanzerChasm
