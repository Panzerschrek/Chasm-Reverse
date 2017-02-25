#pragma once

#include "../connection_info.hpp"
#include "../loopback_buffer.hpp"
#include "../rendering_context.hpp"
#include "../system_event.hpp"
#include "hud_drawer.hpp"
#include "map_drawer.hpp"
#include "map_state.hpp"
#include "minimap_drawer.hpp"
#include "minimap_state.hpp"
#include "movement_controller.hpp"
#include "weapon_state.hpp"

namespace PanzerChasm
{

class Client final
{
public:
	Client(
		Settings& settings,
		const GameResourcesConstPtr& game_resources,
		const MapLoaderPtr& map_loader,
		const RenderingContext& rendering_context,
		const DrawersPtr& drawers,
		const Sound::SoundEnginePtr& sound_engine,
		const DrawLoadingCallback& draw_loading_callback );

	~Client();

	void SetConnection( IConnectionPtr connection );
	bool Disconnected() const;

	void ProcessEvents( const SystemEvents& events );

	void Loop( const KeyboardState& keyboard_state );
	void Draw();

public: // Messages handlers

	// Default handler for non-client messages.
	void operator()( const Messages::MessageBase& message );
	void operator()( const Messages::DummyNetMessage& ) {}

	// Handler for messages, that can be simply transfered to "MapState".
	template<
		class MessageType,
		// Check for appropriate "ProcessMessage" member of "MapState" class.
		class= decltype( std::declval<MapState>().ProcessMessage( MessageType() ) ) >
	void operator()( const MessageType& message )
	{
		if( map_state_ != nullptr )
			map_state_->ProcessMessage( message );
	}

	void operator()( const Messages::PlayerSpawn& message );
	void operator()( const Messages::PlayerPosition& message );
	void operator()( const Messages::PlayerState& message );
	void operator()( const Messages::PlayerWeapon& message );
	void operator()( const Messages::MapEventSound& message );
	void operator()( const Messages::MonsterLinkedSound& message );
	void operator()( const Messages::MonsterSound& message );
	void operator()( const Messages::MapChange& message );
	void operator()( const Messages::TextMessage& message );

private:
	void TrySwitchWeaponOnOutOfAmmo();

private:
	const GameResourcesConstPtr game_resources_;
	const MapLoaderPtr map_loader_;
	const Sound::SoundEnginePtr sound_engine_;
	const DrawLoadingCallback draw_loading_callback_;

	std::unique_ptr<ConnectionInfo> connection_info_;

	// Client uses real time.
	Time current_tick_time_;

	m_Vec3 player_position_;
	EntityId player_monster_id_= 0u;
	Messages::PlayerState player_state_;
	unsigned int requested_weapon_index_= 0u;
	MovementController camera_controller_;
	bool minimap_mode_= false;

	MapDrawer map_drawer_;
	MinimapDrawer minimap_drawer_;
	MapDataConstPtr current_map_data_;
	std::unique_ptr<MapState> map_state_;
	std::unique_ptr<MinimapState> minimap_state_;

	WeaponState weapon_state_;
	bool shoot_pressed_= false;

	HudDrawer hud_drawer_;
};

} // PanzerChasm
