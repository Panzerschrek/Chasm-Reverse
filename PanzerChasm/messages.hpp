#pragma once
#include <limits>

#include <vec.hpp>

#include "game_constants.hpp"

namespace PanzerChasm
{

enum class MessageId : unsigned char
{
	Unknown= 0u,

	#define MESSAGE_FUNC(x) x,
	#include "messages_list.h"
	#undef MESSAGE_FUNC

	// Put it last here
	NumMessages,
};

namespace Messages
{

typedef unsigned short EntityId;
typedef short CoordType;
typedef unsigned short AngleType;

#pragma pack(push, 1)

#define DEFINE_MESSAGE_CONSTRUCTOR(class_name)\
	class_name() { message_id= MessageId::class_name; }

struct MessageBase
{
	MessageId message_id;
};

struct MonsterState : public MessageBase
{
	DEFINE_MESSAGE_CONSTRUCTOR(MonsterState)

	EntityId monster_id;
	CoordType xyz[3];
	AngleType angle;
	unsigned char monster_type;
	unsigned short animation;
	unsigned short animation_frame;
};

struct WallPosition : public MessageBase
{
	DEFINE_MESSAGE_CONSTRUCTOR(WallPosition)

	unsigned short wall_index;
	CoordType vertices_xy[2][2];
	short z;
	unsigned char texture_id;
};

struct PlayerPosition : public MessageBase
{
	DEFINE_MESSAGE_CONSTRUCTOR(PlayerPosition)

	CoordType xyz[3];
	CoordType speed; // Units/s
};

struct PlayerState : public MessageBase
{
	DEFINE_MESSAGE_CONSTRUCTOR(PlayerState)

	typedef unsigned char WeaponsMaskType;
	static_assert(
		std::numeric_limits< WeaponsMaskType >::digits >= GameConstants::weapon_count,
		"Weapons mask type too small" );

	unsigned char ammo[ GameConstants::weapon_count ];
	unsigned char health;
	unsigned char armor;
	unsigned char keys_mask; // Bits 0 - red, 1 - green, 2 - blue.
	WeaponsMaskType weapons_mask;
};

struct PlayerWeapon : public MessageBase
{
	DEFINE_MESSAGE_CONSTRUCTOR(PlayerWeapon)

	unsigned char current_weapon_index_;
	unsigned char animation;
	unsigned char animation_frame;
	unsigned char switch_stage; // 0 - retracted 255 - fully deployed
};

struct ItemState : public MessageBase
{
	DEFINE_MESSAGE_CONSTRUCTOR(ItemState)

	unsigned short item_index;
	CoordType z;
	bool picked;
};

struct StaticModelState : public MessageBase
{
	DEFINE_MESSAGE_CONSTRUCTOR(StaticModelState)

	unsigned short static_model_index;
	CoordType xyz[3];
	AngleType angle;
	unsigned short animation_frame;

	// If true, client must play looped animation starts from frame.
	// If false - only one model frame drawn.
	bool animation_playing;

	unsigned char model_id;
};

struct SpriteEffectBirth : public MessageBase
{
	DEFINE_MESSAGE_CONSTRUCTOR(SpriteEffectBirth)

	CoordType xyz[3];
	unsigned char effect_id;
};

struct ParticleEffectBirth : public MessageBase
{
	DEFINE_MESSAGE_CONSTRUCTOR(ParticleEffectBirth)

	CoordType xyz[3];
	unsigned char effect_id;
};

struct RocketState : public MessageBase
{
	DEFINE_MESSAGE_CONSTRUCTOR(RocketState)

	EntityId rocket_id;
	CoordType xyz[3];
	AngleType angle[2];
};

struct RocketBirth : public RocketState
{
	DEFINE_MESSAGE_CONSTRUCTOR(RocketBirth)

	unsigned char rocket_type;
};

struct RocketDeath : public MessageBase
{
	DEFINE_MESSAGE_CONSTRUCTOR(RocketDeath)

	EntityId rocket_id;
};

struct MapChange : public MessageBase
{
	DEFINE_MESSAGE_CONSTRUCTOR(MapChange)

	unsigned int map_number;
};

struct MonsterBirth : public MessageBase
{
	DEFINE_MESSAGE_CONSTRUCTOR(MonsterBirth)

	EntityId monster_id;
	MonsterState initial_state;
};

struct MonsterDeath : public MessageBase
{
	DEFINE_MESSAGE_CONSTRUCTOR(MonsterDeath)

	EntityId monster_id;
};

struct TextMessage : public MessageBase
{
	DEFINE_MESSAGE_CONSTRUCTOR(TextMessage)

	unsigned short text_message_number;
};

struct PlayerMove : public MessageBase
{
	DEFINE_MESSAGE_CONSTRUCTOR(PlayerMove)

	AngleType view_direction;
	AngleType move_direction;
	unsigned char acceleration; // 0 - stay, 128 - walk, 255 - run
	bool jump_pressed;
	unsigned char weapon_index;
	AngleType view_dir_angle_x;
	AngleType view_dir_angle_z;
	bool shoot_pressed;
};

#undef DEFINE_MESSAGE_CONSTRUCTOR

#pragma pack(pop)

} // namespace Messages

Messages::CoordType CoordToMessageCoord( float x );
float MessageCoordToCoord( Messages::CoordType x );

void PositionToMessagePosition( const m_Vec2& pos, Messages::CoordType* out_pos );
void PositionToMessagePosition( const m_Vec3& pos, Messages::CoordType* out_pos );

void MessagePositionToPosition( const Messages::CoordType* pos, m_Vec2& out_pos );
void MessagePositionToPosition( const Messages::CoordType* pos, m_Vec3& out_pos );

Messages::AngleType AngleToMessageAngle( float angle );
float MessageAngleToAngle( Messages::AngleType angle );

} // namespace PanzerChasm
