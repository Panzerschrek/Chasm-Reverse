#pragma once

#include "../commands_processor.hpp"
#include "../connection_info.hpp"
#include "../loopback_buffer.hpp"
#include "../rendering_context.hpp"
#include "../system_event.hpp"
#include "map_state.hpp"
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
		CommandsProcessor& commands_processor,
		const GameResourcesConstPtr& game_resources,
		const MapLoaderPtr& map_loader,
		IDrawersFactory& drawers_factory,
		const SharedDrawersPtr& shared_drawers,
		const Sound::SoundEnginePtr& sound_engine,
		const DrawLoadingCallback& draw_loading_callback );

	~Client();

	// You should call VidClear before VidRestart.
	// You should no call methods of this class between VidClear and VidRestart.
	void VidClear();
	void VidRestart( IDrawersFactory& drawers_factory );

	void Save( SaveLoadBuffer& buffer, SaveComment& out_save_comment );
	void Load( const SaveLoadBuffer& buffer, unsigned int& buffer_pos );

	void SetConnection( IConnectionPtr connection );
	bool Disconnected() const;
	bool PlayingCutscene() const;

	void ProcessEvents( const SystemEvents& events );

	void Loop( const InputState& input_state, bool paused );
	void Draw();

private:
	struct LoadedMinimapState;

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
	void operator()( const Messages::PlayerItemPickup& message );
	void operator()( const Messages::MapEventSound& message );
	void operator()( const Messages::MonsterLinkedSound& message );
	void operator()( const Messages::MonsterSound& message );
	void operator()( const Messages::MapChange& message );
	void operator()( const Messages::TextMessage& message );

private:
	void StopMap();
	void TrySwitchWeaponOnOutOfAmmo();

	void FullMap();

private:
	Settings& settings_;
	const GameResourcesConstPtr game_resources_;
	const MapLoaderPtr map_loader_;
	const Sound::SoundEnginePtr sound_engine_;
	const DrawLoadingCallback draw_loading_callback_;

	CommandsMapConstPtr commands_;

	std::unique_ptr<ConnectionInfo> connection_info_;

	// Client uses real time minus pauses accumulated time.
	Time current_tick_time_;
	Time accumulated_pauses_time_= Time::FromSeconds(0);

	Time pause_start_time_= Time::FromSeconds(0); // Real time
	bool paused_= false;

	m_Vec3 player_position_;
	EntityId player_monster_id_= 0u;
	Messages::PlayerState player_state_;
	unsigned int requested_weapon_index_= 0u;
	MovementController camera_controller_;
	bool minimap_mode_= false;
	bool small_hud_mode_= false;
	bool full_map_= false;

	SharedDrawersPtr shared_drawers_;
	IMapDrawerPtr map_drawer_;
	IMinimapDrawerPtr minimap_drawer_;
	MapDataConstPtr current_map_data_;
	std::unique_ptr<MapState> map_state_;
	std::unique_ptr<MinimapState> minimap_state_;
	std::unique_ptr<LoadedMinimapState> loaded_minimap_state_;

	WeaponState weapon_state_;
	bool shoot_pressed_= false;

	IHudDrawerPtr hud_drawer_;

	std::unique_ptr<CutscenePlayer> cutscene_player_;
};

} // PanzerChasm
