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
	StaticModelState,

	// Reliable server to client
	MapChange,
	EntityBirth,
	EntityDeath,
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
	unsigned short wall_index;
	short vertices_xy[2][2];
	short z;
};

struct PlayerPosition : public MessageBase
{
	short xyz[3];
};

struct StaticModelState : public MessageBase
{
	unsigned short static_model_index;
	short xyz[3];
	unsigned short angle;
	unsigned short animation_frame;

	// If true, client must play looped animation starts from frame.
	// If false - only one model frame drawn.
	bool animation_playing;

	unsigned char model_id;
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

struct TextMessage : public MessageBase
{
	unsigned short text_message_number;
};

struct PlayerMove : public MessageBase
{
	unsigned short angle;
	unsigned char acceleration; // 0 - stay, 128 - walk, 255 - run
	bool jump_pressed;
};

struct PlayerShot : public MessageBase
{
	unsigned short view_dir_angle_x;
	unsigned short view_dir_angle_z;
};

#pragma pack(pop)

} // namespace Messages

} // namespace PanzerChasm
