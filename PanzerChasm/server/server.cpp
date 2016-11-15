#include <matrix.hpp>

#include "../assert.hpp"
#include "../game_constants.hpp"
#include "../log.hpp"
#include "../math_utils.hpp"
#include "../messages_extractor.inl"

#include "server.hpp"

namespace PanzerChasm
{

Server::Server(
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
}

Server::~Server()
{}

void Server::Loop()
{
	while( IConnectionPtr connection= connections_listener_->GetNewConnection() )
	{
		connection_.reset( new ConnectionInfo( std::move( connection ) ) );

		if( map_ != nullptr )
			map_->SpawnPlayer( player_ );

		Messages::MapChange map_change_msg;
		map_change_msg.message_id= MessageId::MapChange;
		map_change_msg.map_number= current_map_number_;

		connection_->messages_sender.SendReliableMessage( map_change_msg );

		if( map_ != nullptr )
			map_->SendMessagesForNewlyConnectedPlayer( connection_->messages_sender );

		connection_->messages_sender.Flush();
	}

	if( connection_ != nullptr )
		connection_->messages_extractor.ProcessMessages( *this );

	// Do server logic
	UpdateTimes();

	player_.Move( last_tick_duration_ );

	if( map_ != nullptr )
	{
		map_->ProcessPlayerPosition( server_accumulated_time_, player_, connection_->messages_sender );
		map_->Tick( server_accumulated_time_ );
	}

	// Send messages
	if( connection_ != nullptr )
	{
		if( map_ != nullptr )
			map_->SendUpdateMessages( connection_->messages_sender );

		Messages::PlayerPosition position_msg;
		position_msg.message_id= MessageId::PlayerPosition;

		for( unsigned int j= 0u; j < 3u; j++ )
			position_msg.xyz[j]= static_cast<short>( player_.Position().ToArr()[j] * 256.0f );

		connection_->messages_sender.SendUnreliableMessage( position_msg );
		connection_->messages_sender.Flush();
	}
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

	map_->SpawnPlayer( player_ );

	if( connection_ != nullptr )
	{
		Messages::MapChange message;
		message.message_id= MessageId::MapChange;
		message.map_number= current_map_number_;

		connection_->messages_sender.SendReliableMessage( message );
		connection_->messages_sender.Flush();

		map_->SendMessagesForNewlyConnectedPlayer( connection_->messages_sender );
		connection_->messages_sender.Flush();
	}
}

void Server::operator()( const Messages::MessageBase& message )
{
	PC_ASSERT(false);
	Log::Warning( "Unknown message for server: ", int(message.message_id) );
}

void Server::operator()( const Messages::PlayerMove& message )
{
	player_.UpdateMovement( message );
}

void Server::operator()( const Messages::PlayerShot& message )
{
	// TODO - clear this

	const m_Vec3 view_vec( 0.0f, 1.0f, 0.0f );

	m_Mat4 x_rotate, z_rotate;

	x_rotate.RotateX( MessageAngleToAngle( message.view_dir_angle_x ) );
	z_rotate.RotateZ( MessageAngleToAngle( message.view_dir_angle_z ) );

	const m_Vec3 view_vec_rotated= view_vec * x_rotate * z_rotate;

	map_->Shoot(
		0u,
		player_.Position() + m_Vec3( 0.0f, 0.0f, GameConstants::player_eyes_level ),
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

} // namespace PanzerChasm
