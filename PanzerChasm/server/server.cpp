#include "../assert.hpp"
#include "../log.hpp"
#include "../messages_extractor.inl"

#include "server.hpp"

namespace PanzerChasm
{

Server::ConnectionInfo::ConnectionInfo( const IConnectionPtr& in_connection )
	: connection( in_connection )
	, messages_extractor( in_connection )
	, messages_sender( in_connection )
{}

Server::Server(
	const GameResourcesConstPtr& game_resources,
	const MapLoaderPtr& map_loader,
	const IConnectionsListenerPtr& connections_listener )
	: game_resources_(game_resources)
	, map_loader_(map_loader)
	, connections_listener_(connections_listener)
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
	}

	if( connection_ != nullptr )
		connection_->messages_extractor.ProcessMessages( *this );
}

void Server::ChangeMap( const unsigned int map_number )
{
	const MapDataConstPtr map_data= map_loader_->LoadMap( map_number );

	if( map_data == nullptr )
		return;

	state_= State::PlayingMap;
}

void Server::operator()( const Messages::MessageBase& message )
{
	PC_ASSERT(false);
	Log::Warning( "Unknown message for server: ", int(message.message_id) );
}

} // namespace PanzerChasm
