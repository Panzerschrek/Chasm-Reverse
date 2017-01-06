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
	virtual ~Player() override;

	virtual void Tick(
		Map& map,
		EntityId monster_id,
		Time current_time,
		Time last_tick_delta ) override;

	virtual void Hit(
		int damage,
		Map& map,
		EntityId monster_id,
		Time current_time ) override;

	virtual void ClampSpeed( const m_Vec3& clamp_surface_normal ) override;
	virtual void SetOnFloor( bool on_floor ) override;
	virtual void Teleport( const m_Vec3& pos, float angle ) override;

	void SetRandomGenerator( const LongRandPtr& random_generator );

	bool TryActivateProcedure( unsigned int proc_number, Time current_time );
	void ResetActivatedProcedure();

	bool TryPickupItem( unsigned int item_id );

	void BuildPositionMessage( Messages::PlayerPosition& out_position_message ) const;
	void BuildStateMessage( Messages::PlayerState& out_state_message ) const;
	void BuildWeaponMessage( Messages::PlayerWeapon& out_weapon_message ) const;
	bool BuildSpawnMessage( Messages::PlayerSpawn& out_spawn_message ) const; // returns tru, if newly-spawned

	void OnMapChange();
	void UpdateMovement( const Messages::PlayerMove& move_message );

	void SetNoclip( bool noclip );
	bool IsNoclip() const;

	void GiveWeapon();

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

private:
	enum class WeaponState
	{
		Idle,
		Reloading,
		Switching,
	};

private:
	const Time spawn_time_;

	LongRandPtr random_generator_;

	m_Vec3 speed_;
	bool on_floor_;
	bool noclip_;
	bool teleported_;

	unsigned char ammo_[ GameConstants::weapon_count ];
	bool have_weapon_[ GameConstants::weapon_count ];
	int health_;
	int armor_;

	bool have_red_key_= false;
	bool have_green_key_= false;
	bool have_blue_key_= false;

	float mevement_acceleration_= 0.0f;
	float movement_direction_= 0.0f;
	bool jump_pessed_= false;

	WeaponState weapon_state_= WeaponState::Idle;

	float view_angle_x_= 0.0f;
	float view_angle_z_= 0.0f;
	bool shoot_pressed_= false;
	Time last_shoot_time_;

	unsigned int current_weapon_index_= 0u;
	unsigned int requested_weapon_index_= 0u;
	float weapon_switch_stage_= 1.0f; // 0 - retracted, 1 - fully deployed

	unsigned int current_weapon_animation_= 0u;
	unsigned int current_weapon_animation_frame_= 0u;
	Time weapon_animation_state_change_time_;

	Time last_pain_sound_time_;
	Time last_step_sound_time_;

	unsigned int last_activated_procedure_= ~0u; // Inv zero is dummy.
	Time last_activated_procedure_activation_time_= Time::FromSeconds(0);
};

} // namespace PanzerChasm
