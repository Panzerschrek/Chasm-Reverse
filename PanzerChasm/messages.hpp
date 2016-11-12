#pragma once

#include <vec.hpp>

namespace PanzerChasm
{

enum class MessageId : unsigned char
{
	Unknown= 0u,

	// Unreliable server to client
	MonsterState,
	WallPosition,
	PlayerPosition, // position of player, which recieve this message.
	StaticModelState,
	SpriteEffectBirth,

	// Reliable server to client
	MapChange,
	MonsterBirth,
	MonsterDeath,
	TextMessage,

	// Unrealiable client to server
	PlayerMove,

	// Reliable client to server
	PlayerShot,

	// Put it last here
	NumMessages,
};

namespace Messages
{

typedef unsigned short EntityId;
typedef short CoordType;
typedef unsigned short AngleType;

#pragma pack(push, 1)

struct MessageBase
{
	MessageId message_id;
};

struct MonsterState : public MessageBase
{
	EntityId monster_id;
	CoordType xyz[3];
	AngleType angle;
	unsigned char monster_type;
	unsigned short animation;
	unsigned short animation_frame;
};

struct WallPosition : public MessageBase
{
	unsigned short wall_index;
	CoordType vertices_xy[2][2];
	short z;
};

struct PlayerPosition : public MessageBase
{
	CoordType xyz[3];
};

struct StaticModelState : public MessageBase
{
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
	CoordType xyz[3];
	unsigned char effect_id;
};

struct MapChange : public MessageBase
{
	unsigned int map_number;
};

struct MonsterBirth : public MessageBase
{
	EntityId monster_id;
	MonsterState initial_state;
};

struct MonsterDeath : public MessageBase
{
	EntityId monster_id;
};

struct TextMessage : public MessageBase
{
	unsigned short text_message_number;
};

struct PlayerMove : public MessageBase
{
	AngleType angle;
	unsigned char acceleration; // 0 - stay, 128 - walk, 255 - run
	bool jump_pressed;
};

struct PlayerShot : public MessageBase
{
	AngleType view_dir_angle_x;
	AngleType view_dir_angle_z;
};

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
