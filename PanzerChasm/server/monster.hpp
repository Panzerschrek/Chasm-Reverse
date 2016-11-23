#pragma once
#include <memory>

#include <vec.hpp>

#include "../game_resources.hpp"
#include "../map_loader.hpp"
#include "../time.hpp"

namespace PanzerChasm
{

class Monster final
{
public:
	Monster(
		const MapData::Monster& map_monster,
		float z,
		const GameResourcesConstPtr& game_resources,
		Time spawn_time );
	~Monster();

	unsigned char MonsterId() const;
	const m_Vec3& Position() const;
	float Angle() const;

	unsigned int CurrentAnimation() const;
	unsigned int CurrentAnimationFrame() const;

	void Tick( Time current_time );
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
		Idle1= 3u,
		RemoteAttack0= 4u,
		RemoteAttack1= 5u,
		MeleeAtack0= 6u,
		MeleeAtack1= 7u,
		MeleeAtack2= 8u,
		Pain0=  9u,
		Pain1= 10u,
		RightHandLost= 11u,
		LeftHandLost= 12u,
		HeadLost= 13u, // ?
		Death0= 14u, // Reverse death animation, possibly
		Death1= 15u,
		Death2= 16u,
		Death3= 17u,
	};

private:
	// returns -1 if not found
	int GetAnimation( AnimationId id ) const;
	int GetAnyAnimation( const std::initializer_list<AnimationId>& ids ) const;

private:
	const unsigned char monster_id_;
	const GameResourcesConstPtr game_resources_;

	m_Vec3 pos_;
	float angle_;
	State state_= State::Idle;
	int life_;

	unsigned int current_animation_= 0u;
	unsigned int current_animation_frame_= 0u;
	Time current_animation_start_time_;
};

typedef std::unique_ptr<Monster> MonsterPtr;

} // namespace PanzerChasm
