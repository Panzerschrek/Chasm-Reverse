#pragma once

#include "../connection_info.hpp"
#include "../loopback_buffer.hpp"
#include "../rendering_context.hpp"
#include "../system_event.hpp"
#include "hud_drawer.hpp"
#include "map_drawer.hpp"
#include "map_state.hpp"
#include "movement_controller.hpp"
#include "weapon_state.hpp"

namespace PanzerChasm
{

class Client final
{
public:
	Client(
		const GameResourcesConstPtr& game_resources,
		const MapLoaderPtr& map_loader,
		const LoopbackBufferPtr& loopback_buffer,
		const RenderingContext& rendering_context,
		const DrawersPtr& drawers,
		const Sound::SoundEnginePtr& sound_engine );

	~Client();

	void ProcessEvents( const SystemEvents& events );

	void Loop();
	void Draw();

public: // Messages handlers
	void operator()( const Messages::MessageBase& message );
	void operator()( const Messages::MonsterState& message );
	void operator()( const Messages::WallPosition& message );
	void operator()( const Messages::PlayerSpawn& message );
	void operator()( const Messages::PlayerPosition& message );
	void operator()( const Messages::PlayerState& message );
	void operator()( const Messages::PlayerWeapon& message );
	void operator()( const Messages::ItemState& message );
	void operator()( const Messages::StaticModelState& message );
	void operator()( const Messages::SpriteEffectBirth& message );
	void operator()( const Messages::ParticleEffectBirth& message );
	void operator()( const Messages::MapEventSound& message );
	void operator()( const Messages::MapChange& message );
	void operator()( const Messages::MonsterBirth& message );
	void operator()( const Messages::MonsterDeath& message );
	void operator()( const Messages::TextMessage& message );
	void operator()( const Messages::RocketState& message );
	void operator()( const Messages::RocketBirth& message );
	void operator()( const Messages::RocketDeath& message );

private:
	void TrySwitchWeaponOnOutOfAmmo();

private:
	const GameResourcesConstPtr game_resources_;
	const MapLoaderPtr map_loader_;
	const LoopbackBufferPtr loopback_buffer_;
	const Sound::SoundEnginePtr sound_engine_;

	std::unique_ptr<ConnectionInfo> connection_info_;

	// Client uses real time.
	Time current_tick_time_;

	m_Vec3 player_position_;
	Messages::EntityId player_monster_id_= 0u;
	Messages::PlayerState player_state_;
	unsigned int requested_weapon_index_= 0u;
	MovementController camera_controller_;
	bool map_mode_= false;

	MapDrawer map_drawer_;
	MapDataConstPtr current_map_data_;
	std::unique_ptr<MapState> map_state_;

	WeaponState weapon_state_;
	bool shoot_pressed_= false;

	HudDrawer hud_drawer_;
};

} // PanzerChasm
