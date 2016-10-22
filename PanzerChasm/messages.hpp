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
	EntityBirth,
	EntityDeath,

	// Unrealiable client to server
	PlayerMove,

	// Reliable client to server
	PlayerShot,
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


class IClientMessagesProcessor
{

	virtual void Process( const PlayerMove& entity_state )= 0;

	virtual void Process( const PlayerShot& entity_state )= 0;
};

class IServerMessagesProcessor
{
	virtual void Process( const EntityState& entity_state )= 0;
	virtual void Process( const WallPosition& entity_state )= 0;
	virtual void Process( const PlayerPosition& entity_state )= 0;

	virtual void Process( const EntityBirth& entity_state )= 0;
	virtual void Process( const EntityDeath& entity_state )= 0;

};

class IClientMessagesExtractor
{
public:
	ProcessMessages( IClientMessagesProcessor& messages_processor )= 0;
};

class IServerMessagesExtractor
{
public:
	ProcessMessages( IServerMessagesProcessor& messages_processor )= 0;
};

} // namespace Messages

} // namespace PanzerChasm
