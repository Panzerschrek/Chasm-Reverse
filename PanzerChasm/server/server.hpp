#pragma once

#include "../commands_processor.hpp"
#include "../connection_info.hpp"
#include "../time.hpp"
#include "i_connections_listener.hpp"
#include "fwd.hpp"
#include "map.hpp"

namespace PanzerChasm
{

class Server final
{
public:
	Server(
		CommandsProcessor& commands_processor,
		const GameResourcesConstPtr& game_resources,
		const MapLoaderPtr& map_loader,
		const IConnectionsListenerPtr& connections_listener,
		const DrawLoadingCallback& draw_loading_callback );
	~Server();

	void Loop( bool paused );

	// Returns true, if map successfully changed or restarted.
	bool ChangeMap( unsigned int map_number, DifficultyType difficulty, GameRules game_rules );
	void StopMap();

	void Save( SaveLoadBuffer& buffer );
	bool Load( const SaveLoadBuffer& buffer, unsigned int& buffer_pos );

	void DisconnectAllClients();

public: // Messages handlers
	void operator()( const Messages::MessageBase& message );
	void operator()( const Messages::DummyNetMessage& ) {}
	void operator()( const Messages::PlayerMove& message );

private:
	struct ConnectedPlayer final
	{
		ConnectedPlayer(
			const IConnectionPtr& connection,
			const GameResourcesConstPtr& game_resoruces,
			Time current_time );

		ConnectionInfo connection_info;
		PlayerPtr player;
		EntityId player_monster_id;
	};

	typedef std::unique_ptr<ConnectedPlayer> ConnectedPlayerPtr;

private:
	void UpdateTimes();

	void GiveAmmo();
	void GiveArmor();
	void GiveWeapon();
	void GiveKeys();
	void ToggleGodMode();
	void ToggleNoclip();

private:
	const GameResourcesConstPtr game_resources_;
	const MapLoaderPtr map_loader_;
	const IConnectionsListenerPtr connections_listener_;
	const DrawLoadingCallback draw_loading_callback_;

	const Map::MapEndCallback map_end_callback_;

	CommandsMapConstPtr commands_;

	GameRules game_rules_= GameRules::SinglePlayer;
	unsigned int current_map_number_= ~0;
	MapDataConstPtr current_map_data_;
	std::unique_ptr<Map> map_;

	bool map_end_triggered_= false;
	bool join_first_client_with_existing_player_= false;

	std::vector<ConnectedPlayerPtr> players_;
	ConnectedPlayer* current_player_= nullptr;

	Time last_tick_; // Real time
	Time server_accumulated_time_; // Not real, can be faster or slower.
	Time last_tick_duration_;

	// Cheats
	bool noclip_= false;
	bool god_mode_= false;
};

} // PanzerChasm
