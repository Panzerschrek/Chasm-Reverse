#pragma once

#include <vec.hpp>

#include "../game_constants.hpp"
#include "../game_resources.hpp"
#include "../messages.hpp"
#include "../time.hpp"
#include "a_code.hpp"
#include "monster_base.hpp"

namespace PanzerChasm
{

// Server side player logic here.
class Player final : public MonsterBase
{
public:
	Player( const GameResourcesConstPtr& game_resources, Time current_time );
	Player( const GameResourcesConstPtr& game_resources, LoadStream& load_stream );
	virtual ~Player() override;

	virtual void Save( SaveStream& save_stream ) override;

	virtual void Tick(
		Map& map,
		EntityId monster_id,
		Time current_time,
		Time last_tick_delta ) override;

	virtual void Hit(
		int damage,
		const m_Vec2& hit_direction,
		EntityId opponent_id, // Zero from damage from environment
		Map& map,
		EntityId monster_id,
		Time current_time ) override;

	virtual void TryWarn(
		const m_Vec3& pos,
		Map& map,
		EntityId monster_id,
		Time current_time ) override;

	virtual void ClampSpeed( const m_Vec3& clamp_surface_normal ) override;
	virtual void SetOnFloor( bool on_floor ) override;
	virtual void Teleport( const m_Vec3& pos, float angle ) override;
	virtual bool IsFullyDead() const override;
	virtual unsigned char GetColor() const override;

	void SetRandomGenerator( const LongRandPtr& random_generator );

	bool TryActivateProcedure( unsigned int proc_number, Time current_time );
	void ResetActivatedProcedure();

	bool TryPickupItem( unsigned int item_id, Time current_time );
	bool TryPickupBackpack( const Backpack& backpack );

	void BuildPositionMessage( Messages::PlayerPosition& out_position_message ) const;
	void BuildStateMessage( Messages::PlayerState& out_state_message ) const;
	void BuildWeaponMessage( Messages::PlayerWeapon& out_weapon_message ) const;
	bool BuildSpawnMessage( Messages::PlayerSpawn& out_spawn_message, bool force= false ) const; // returns true, if newly-spawned
	void SendInternalMessages( MessagesSender& messages_sender );

	void OnMapChange();
	void UpdateMovement( const Messages::PlayerMove& move_message );

	void SetNoclip( bool noclip );
	bool IsNoclip() const;

	void SetGodMode( bool god_mode );

	void GiveWeapon();
	void GiveAmmo();
	void GiveArmor();

	void GiveRedKey();
	void GiveGreenKey();
	void GiveBlueKey();
	void GiveAllKeys();

	bool HaveRedKey() const;
	bool HaveGreenKey() const;
	bool HaveBlueKey() const;

	unsigned int CurrentWeaponIndex() const;

	unsigned int CurrentAnimation() const;
	unsigned int CurrentAnimationFrame() const;

private:
	// Returns true, if jumped.
	bool Move( Time time_delta );
	void GenItemPickupMessage( unsigned char item_id );
	void AddItemPickupFlash();

private:
	enum class State
	{
		Alive,
		DeathAnimation,
		Dead,
	};

	enum class WeaponState
	{
		Idle,
		Reloading,
		Switching,
	};

private:
	Time spawn_time_= Time::FromSeconds(0);

	LongRandPtr random_generator_;

	m_Vec3 speed_;
	bool on_floor_;
	bool noclip_;
	bool god_mode_;
	bool teleported_;

	unsigned char ammo_[ GameConstants::weapon_count ];
	bool have_weapon_[ GameConstants::weapon_count ];
	int armor_;

	bool have_red_key_= false;
	bool have_green_key_= false;
	bool have_blue_key_= false;

	float mevement_acceleration_= 0.0f;
	float movement_direction_= 0.0f;
	bool jump_pessed_= false;

	State state_= State::Alive;
	Time last_state_change_time_= Time::FromSeconds(0);

	WeaponState weapon_state_= WeaponState::Switching;

	float view_angle_x_= 0.0f;
	float view_angle_z_= 0.0f;
	bool shoot_pressed_= false;
	Time last_shoot_time_= Time::FromSeconds(0);

	unsigned int current_weapon_index_= 0u;
	unsigned int requested_weapon_index_= 0u;
	float weapon_switch_stage_= 0.0f; // 0 - retracted, 1 - fully deployed. Start semi-deployed.

	unsigned int current_weapon_animation_= 0u;
	unsigned int current_weapon_animation_frame_= 0u;
	Time weapon_animation_state_change_time_= Time::FromSeconds(0);

	Time invisibility_take_time_= Time::FromSeconds(0);
	bool has_invisibility_= false;

	Time last_pain_sound_time_= Time::FromSeconds(0);
	Time last_step_sound_time_= Time::FromSeconds(0);

	unsigned int last_activated_procedure_= ~0u; // Inv zero is dummy.
	Time last_activated_procedure_activation_time_= Time::FromSeconds(0);

	std::vector<Messages::PlayerItemPickup> pickup_messages_;
	std::vector<Messages::FullscreenBlendEffect> fullscreen_blend_messages_;
};

} // namespace PanzerChasm
