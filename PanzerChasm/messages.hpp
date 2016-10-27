#pragma once

namespace PanzerChasm
{

typedef unsigned short EntityId;

enum class MessageId : unsigned char
{
	Unknown= 0u,

	// Unreliable server to client
	EntityState, // State of mosters, items, other players.
	WallPosition,
	PlayerPosition, // position of player, which recieve this message.

	// Reliable server to client
	MapChange,
	EntityBirth,
	EntityDeath,

	// Unrealiable client to server
	PlayerMove,

	// Reliable client to server
	PlayerShot,

	// Put it last here
	NumMessages,
};

namespace Messages
{

#pragma pack(push, 1)

struct MessageBase
{
	MessageId message_id;
};

struct EntityState : public MessageBase
{
	EntityId id;
	short xyz[3];
	unsigned short angle;
	unsigned short animation_frame[2];
	unsigned char animation_frames_mix;
};

struct WallPosition : public MessageBase
{
	unsigned char wall_xy[2]; // wall coordinate
	short vertices_xy[2][2];
	short z;
};

struct PlayerPosition : public MessageBase
{
	short xyz[3];
};

struct MapChange : public MessageBase
{
	unsigned int map_number;
};

struct EntityBirth : public MessageBase
{
	EntityId id;
	EntityState initial_state;
};

struct EntityDeath : public MessageBase
{
	EntityId id;
};

struct PlayerMove : public MessageBase
{
	unsigned short angle;
	unsigned char acceleration; // 0 - stay, 128 - walk, 255 - run
};

struct PlayerShot : public MessageBase
{
	unsigned short view_dir_angle_x;
	unsigned short view_dir_angle_z;
};

#pragma pack(pop)

} // namespace Messages

} // namespace PanzerChasm
