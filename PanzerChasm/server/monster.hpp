#pragma once
#include <memory>

#include <vec.hpp>

#include "../game_resources.hpp"
#include "../map_loader.hpp"
#include "../time.hpp"
#include "fwd.hpp"
#include "monster_base.hpp"
#include "rand.hpp"

namespace PanzerChasm
{

class Monster final : public MonsterBase
{
public:
	Monster(
		const MapData::Monster& map_monster,
		float z,
		const GameResourcesConstPtr& game_resources,
		const LongRandPtr& random_generator,
		Time spawn_time );

	virtual ~Monster() override;

	virtual void Tick( Map& map, Time current_time, Time last_tick_delta ) override;
	virtual void Hit( int damage, Time current_time ) override;

	virtual void ClampSpeed( const m_Vec3& clamp_surface_normal ) override;
	virtual void SetOnFloor( bool on_floor ) override;

private:
	enum class State
	{
		Idle,
		MoveToTarget,
		PainShock,
		RemoteAttack,
		MeleeAttack,
		DeathAnimation,
		Dead,
	};

private:
	void FallDown( float time_delta_s );
	void MoveToTarget( float time_delta_s );
	void RotateToTarget( float time_delta_s );
	bool SelectTarget( const Map& map, Time current_time ); // returns true, if selected

private:
	LongRandPtr random_generator_;

	State state_= State::Idle;

	Time current_animation_start_time_;

	float vertical_speed_= 0.0f;

	bool attack_was_done_;

	PlayerConstWeakPtr target_;
	m_Vec3 target_position_;
	Time target_change_time_= Time::FromSeconds(0);
};

} // namespace PanzerChasm
