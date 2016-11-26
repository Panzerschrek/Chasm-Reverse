#pragma once
#include <memory>

#include <vec.hpp>

#include "../game_resources.hpp"
#include "../map_loader.hpp"
#include "../time.hpp"
#include "rand.hpp"

namespace PanzerChasm
{

class Monster final
{
public:
	Monster(
		const MapData::Monster& map_monster,
		float z,
		const GameResourcesConstPtr& game_resources,
		const LongRandPtr& random_generator,
		Time spawn_time );
	~Monster();

	unsigned char MonsterId() const;
	const m_Vec3& Position() const;
	float Angle() const;

	unsigned int CurrentAnimation() const;
	unsigned int CurrentAnimationFrame() const;

	void Tick( Time current_time, Time last_tick_delta );
	void Hit( int damage, Time current_time );

	bool TryShot( const m_Vec3& from, const m_Vec3& direction_normalized, m_Vec3& out_pos ) const;

private:
	enum class State
	{
		Idle,
		MoveToTarget,
		PainShock,
		WaitAfterAttack,
		DeathAnimation,
		Dead,
	};

	// TODO - check this. Some numbers may be incorrect.
	enum class AnimationId : unsigned int
	{
		Run= 0u,
		Idle0= 2u,
		Idle1= 3u, // Look around, check weapon, etc.
		Boring= 4u,
		RemoteAttack= 5u,
		MeleeAttackLeftHand= 6u,
		MeleeAttackRightHand= 7u,
		MeleeAttackHead= 8u,
		Pain0=  9u,
		Pain1= 10u, // More painful
		RightHandLost= 11u,
		LeftHandLost= 12u,
		HeadLost= 13u, // Head lost without death, possibly
		Death0= 14u, // Reverse death animation, possibly
		Death1= 15u,
		Death2= 16u,
		Death3= 17u,
	};

private:
	// returns -1 if not found
	int GetAnimation( AnimationId id ) const;
	int GetAnyAnimation( const std::initializer_list<AnimationId>& ids ) const;

	void MoveToTarget( const float time_delta_s );
	void SelectTarget( Time current_time );

private:
	const unsigned char monster_id_;
	const GameResourcesConstPtr game_resources_;
	LongRandPtr random_generator_;

	m_Vec3 pos_;
	float angle_; // [ 0; 2 * pi )
	State state_= State::Idle;
	int life_;

	unsigned int current_animation_= 0u;
	unsigned int current_animation_frame_= 0u;
	Time current_animation_start_time_;

	m_Vec3 target_position_;
	Time target_change_time_= Time::FromSeconds(0);
};

typedef std::unique_ptr<Monster> MonsterPtr;

} // namespace PanzerChasm
