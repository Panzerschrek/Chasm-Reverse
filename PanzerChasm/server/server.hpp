#pragma once
#include <chrono>

#include "../game_resources.hpp"
#include "../map_loader.hpp"
#include "../messages_extractor.hpp"
#include "../messages_sender.hpp"
#include "i_connections_listener.hpp"

namespace PanzerChasm
{

class Server final
{
public:
	Server(
		const GameResourcesConstPtr& game_resources,
		const MapLoaderPtr& map_loader,
		const IConnectionsListenerPtr& connections_listener );
	~Server();

	void Loop();

	void ChangeMap( unsigned int map_number );

public: // Messages handlers
	void operator()( const Messages::MessageBase& message );
	void operator()( const Messages::PlayerMove& message );

private:
	struct ConnectionInfo
	{
		explicit ConnectionInfo( const IConnectionPtr& in_connection );

		IConnectionPtr connection;
		MessagesExtractor messages_extractor;
		MessagesSender messages_sender;
	};

	enum class State
	{
		NoMap,
		PlayingMap,
	};

private:
	const GameResourcesConstPtr game_resources_;
	const MapLoaderPtr map_loader_;
	const IConnectionsListenerPtr connections_listener_;

	State state_= State::NoMap;

	std::unique_ptr<ConnectionInfo> connection_;

	std::chrono::milliseconds last_tick_;
	float last_tick_duration_s_;

	m_Vec3 player_pos_;
};


} // PanzerChasm
