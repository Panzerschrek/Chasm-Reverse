#include "../assert.hpp"
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
	, startup_time_( Time::CurrentTime() )
	, last_tick_( Time::CurrentTime() )
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

		Messages::MapChange map_change_msg;
		map_change_msg.message_id= MessageId::MapChange;
		map_change_msg.map_number= current_map_number_;

		connection_->messages_sender.SendReliableMessage( map_change_msg );
		connection_->messages_sender.Flush();

		if( current_map_data_ != nullptr )
			for( const MapData::Monster& monster : current_map_data_->monsters )
			{
				if( monster.monster_id == 0u && monster.difficulty_flags == 0 )
				{
					player_.SetPosition( m_Vec3( monster.pos, 0.0f ) );
					break;
				}
			}
	}

	if( connection_ != nullptr )
		connection_->messages_extractor.ProcessMessages( *this );

	const Time current_time= Time::CurrentTime();
	Time dt= current_time - last_tick_;
	dt= std::min( dt, Time::FromSeconds( 0.060 ) );
	dt= std::max( dt, Time::FromSeconds( 0.002 ) );
	last_tick_duration_= dt;

	last_tick_= current_time;

	// Do server logic

	player_.Move( last_tick_duration_ );

	if( map_ != nullptr )
	{
		map_->ProcessPlayerPosition( current_time, player_, connection_->messages_sender );
		map_->Tick( current_time );
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
		return;

	current_map_number_= map_number;
	current_map_data_= map_data;
	map_.reset( new Map( map_data, last_tick_ ) );

	state_= State::PlayingMap;
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

} // namespace PanzerChasm
