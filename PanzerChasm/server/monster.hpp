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
		const GameResourcesConstPtr& game_resources,
		Time spawn_time );
	~Monster();

	unsigned char MonsterId() const;
	const m_Vec3& Position() const;
	float Angle() const;

	unsigned int CurrentAnimation() const;
	unsigned int CurrentAnimationFrame() const;

	void Tick( Time current_time );

private:
	enum class State
	{
		Idle,
		MoveToTarget,
		PainShock,
		WaitAfterAttack,
	};

	// TODO - check this. Some numbers may be incorrect.
	enum class AnimationId : unsigned int
	{
		Run= 0u,
		Idle= 2u,
		RemoteAttack0= 4u,
		RemoteAttack1= 5u,
		MeleeAtack0= 6u,
		MeleeAtack1= 7u,
		MeleeAtack2= 8u,
		Pain0=  9u,
		Pain1= 10u,
		RightHandLost= 11u,
		LeftHandLost= 12u,
		Death0= 13u,
		Death1= 14u,
	};

private:
	unsigned int GetAnimation( AnimationId id ) const;

private:
	const unsigned char monster_id_;
	const GameResourcesConstPtr game_resources_;

	m_Vec3 pos_;
	float angle_;
	State state_= State::Idle;

	unsigned int current_animation_= 0u;
	unsigned int current_animation_frame_= 0u;
	Time current_animation_start_time_;
};

typedef std::unique_ptr<Monster> MonsterPtr;

} // namespace PanzerChasm
