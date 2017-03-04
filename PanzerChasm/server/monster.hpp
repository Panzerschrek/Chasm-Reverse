#pragma once
#include <memory>

#include <vec.hpp>

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
	Monster(
		unsigned char monster_id,
		const GameResourcesConstPtr& game_resources,
		const LongRandPtr& random_generator,
		LoadStream& load_stream );

	virtual ~Monster() override;

	virtual void Save( SaveStream& save_stream ) override;

	virtual void Tick(
		Map& map,
		EntityId monster_id,
		Time current_time,
		Time last_tick_delta ) override;

	virtual void Hit(
		int damage,
		const m_Vec2& hit_direction,
		Map& map,
		EntityId monster_id,
		Time current_time ) override;

	virtual void ClampSpeed( const m_Vec3& clamp_surface_normal ) override;
	virtual void SetOnFloor( bool on_floor ) override;
	virtual void Teleport( const m_Vec3& pos, float angle ) override;

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
	bool IsBoss() const;

	void FallDown( float time_delta_s );
	void MoveToTarget( float time_delta_s );
	void RotateToTarget( float time_delta_s );
	bool SelectTarget( const Map& map, Time current_time ); // returns true, if selected
	int SelectMeleeAttackAnimation();
	void SpawnBodyPart( Map& map, unsigned char part_id );

private:
	LongRandPtr random_generator_;

	State state_= State::Idle;

	Time current_animation_start_time_= Time::FromSeconds(0);

	float vertical_speed_= 0.0f;

	bool attack_was_done_;

	struct
	{
		EntityId monster_id= 0u;
		MonsterBaseWeakPtr monster;
	} target_;

	m_Vec3 target_position_;
	Time target_change_time_= Time::FromSeconds(0);
};

} // namespace PanzerChasm
