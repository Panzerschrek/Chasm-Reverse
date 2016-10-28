#include "../assert.hpp"
#include "../log.hpp"
#include "../math_utils.hpp"
#include "../messages_extractor.inl"

#include "server.hpp"

namespace PanzerChasm
{

static std::chrono::milliseconds GetTime()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch());
}

Server::Server(
	const GameResourcesConstPtr& game_resources,
	const MapLoaderPtr& map_loader,
	const IConnectionsListenerPtr& connections_listener )
	: game_resources_(game_resources)
	, map_loader_(map_loader)
	, connections_listener_(connections_listener)
	, startup_time_( GetTime() )
	, last_tick_( GetTime() )
	, player_pos_( 0.0f, 0.0f, 0.0f )
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
				if( monster.monster_id == 0u )
				{
					player_pos_= m_Vec3( monster.pos, 0.0f );
					break;
				}
			}
	}

	if( connection_ != nullptr )
		connection_->messages_extractor.ProcessMessages( *this );

	const std::chrono::milliseconds current_time= GetTime();
	unsigned int dt_ms= ( current_time - last_tick_ ).count();
	if( dt_ms < 2u ) dt_ms= 2u;
	else if( dt_ms >= 60u ) dt_ms= 60u;
	last_tick_duration_s_= float(dt_ms) / 1000.0f;

	last_tick_= current_time;

	// Do server logic

	{
		const float c_max_speed= 5.0f;
		const float speed= c_max_speed * player_movement_.acceleration;

		const float delta= last_tick_duration_s_ * speed;

		player_pos_.x+= delta * std::cos(player_movement_.direction);
		player_pos_.y+= delta * std::sin(player_movement_.direction);
	}

	if( map_ != nullptr )
	{
		const Map::TimePoint absolute_time=
			std::chrono::duration_cast<std::chrono::milliseconds>((current_time - startup_time_)).count();

		map_->ProcessPlayerPosition( absolute_time, player_pos_ );
		map_->Tick( absolute_time, last_tick_duration_s_ );
	}

	// Send messages
	if( connection_ != nullptr )
	{
		if( map_ != nullptr )
			map_->SendUpdateMessages( connection_->messages_sender );

		Messages::PlayerPosition position_msg;
		position_msg.message_id= MessageId::PlayerPosition;

		for( unsigned int j= 0u; j < 3u; j++ )
			position_msg.xyz[j]= static_cast<short>( player_pos_.ToArr()[j] * 256.0f );

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
	map_.reset( new Map( map_data ) );

	state_= State::PlayingMap;
}

void Server::operator()( const Messages::MessageBase& message )
{
	PC_ASSERT(false);
	Log::Warning( "Unknown message for server: ", int(message.message_id) );
}

void Server::operator()( const Messages::PlayerMove& message )
{
	player_movement_.acceleration= float(message.acceleration) / 255.0f;
	player_movement_.direction= float(message.angle) / 65536.0f * Constants::two_pi;
}

} // namespace PanzerChasm
