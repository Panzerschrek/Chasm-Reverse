#pragma once

#include "../commands_processor.hpp"
#include "../connection_info.hpp"
#include "../game_resources.hpp"
#include "../map_loader.hpp"
#include "../time.hpp"
#include "i_connections_listener.hpp"
#include "map.hpp"
#include "player.hpp"

namespace PanzerChasm
{

class Server final
{
public:
	Server(
		CommandsProcessor& commands_processor,
		const GameResourcesConstPtr& game_resources,
		const MapLoaderPtr& map_loader,
		const IConnectionsListenerPtr& connections_listener );
	~Server();

	void Loop();

	void ChangeMap( unsigned int map_number );

public: // Messages handlers
	void operator()( const Messages::MessageBase& message );
	void operator()( const Messages::PlayerMove& message );
	void operator()( const Messages::PlayerShot& message );

private:
	enum class State
	{
		NoMap,
		PlayingMap,
	};

private:
	void UpdateTimes();

	void GiveAmmo();
	void GiveArmor();
	void GiveWeapon();
	void ToggleGodMode();
	void ToggleNoclip();

private:
	const GameResourcesConstPtr game_resources_;
	const MapLoaderPtr map_loader_;
	const IConnectionsListenerPtr connections_listener_;

	CommandsMapConstPtr commands_;

	State state_= State::NoMap;
	unsigned int current_map_number_= ~0;
	MapDataConstPtr current_map_data_;
	std::unique_ptr<Map> map_;

	std::unique_ptr<ConnectionInfo> connection_;

	Time last_tick_; // Real time
	Time server_accumulated_time_; // Not real, can be faster or slower.
	Time last_tick_duration_;

	Player player_;
};

} // PanzerChasm
