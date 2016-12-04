#pragma once

#include "../game_resources.hpp"
#include "../time.hpp"

#include "fwd.hpp"

#include <vec.hpp>

namespace PanzerChasm
{

class MonsterBase
{
public:
	MonsterBase(
		const GameResourcesConstPtr& game_resources,
		unsigned char monster_id,
		const m_Vec3& pos, float angle );
	virtual ~MonsterBase();

	unsigned char MonsterId() const;

	const m_Vec3& Position() const;
	void SetPosition( const m_Vec3& pos );

	float Angle() const;

	int Health() const;

	unsigned int CurrentAnimation() const;
	unsigned int CurrentAnimationFrame() const;

	bool TryShot( const m_Vec3& from, const m_Vec3& direction_normalized, m_Vec3& out_pos ) const;

	virtual void Tick( Map& map, Time current_time, Time last_tick_delta )= 0;
	virtual void Hit( int damage, Time current_time )= 0;

	virtual void ClampSpeed( const m_Vec3& clamp_surface_normal )= 0;
	virtual void SetOnFloor( bool on_floor )= 0;

protected:
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

protected:
	// returns -1 if not found
	int GetAnimation( AnimationId id ) const;
	int GetAnyAnimation( const std::initializer_list<AnimationId>& ids ) const;

protected:
	const GameResourcesConstPtr game_resources_;
	const unsigned char monster_id_;

	m_Vec3 pos_;
	float angle_; // [ 0; 2 * pi )

	int health_;

	unsigned int current_animation_= 0u;
	unsigned int current_animation_frame_= 0u;
};

} // namespace PanzerChasm
