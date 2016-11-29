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
	explicit Player( const GameResourcesConstPtr& game_resources );
	~Player();

	void ClampSpeed( const m_Vec3& clamp_surface_normal );
	void SetOnFloor( bool on_floor );

	bool TryActivateProcedure( unsigned int proc_number, Time current_time );
	void ResetActivatedProcedure();

	bool TryPickupItem( unsigned int item_id );

	void BuildStateMessage( Messages::PlayerState& out_state_message ) const;

	void UpdateMovement( const Messages::PlayerMove& move_message );
	void Move( Time time_delta );

	void SetNoclip( bool noclip );
	bool IsNoclip() const;

	void GiveRedKey();
	void GiveGreenKey();
	void GiveBlueKey();
	void GiveAllKeys();

	bool HaveRedKey() const;
	bool HaveGreenKey() const;
	bool HaveBlueKey() const;

private:

	m_Vec3 speed_;
	bool on_floor_;
	bool noclip_;

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

	unsigned int last_activated_procedure_= 0u; // zero is dummy
	Time last_activated_procedure_activation_time_= Time::FromSeconds(0);
};

} // namespace PanzerChasm
