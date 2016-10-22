#pragma once

#include "../game_resources.hpp"
#include "../map_loader.hpp"
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

private:
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
};


} // PanzerChasm
