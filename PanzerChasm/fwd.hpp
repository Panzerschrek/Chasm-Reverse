#pragma once
#include <memory>

// Forward declarations, basic typedefs here.

namespace PanzerChasm
{

class IConnection;
typedef std::shared_ptr<IConnection> IConnectionPtr;

struct Drawers;
typedef std::shared_ptr<Drawers> DrawersPtr;

struct GameResources;
typedef std::shared_ptr<GameResources> GameResourcesPtr;
typedef std::shared_ptr<const GameResources> GameResourcesConstPtr;

class LoopbackBuffer;
typedef  std::shared_ptr<LoopbackBuffer> LoopbackBufferPtr;

struct MapData;
typedef std::shared_ptr<MapData> MapDataPtr;
typedef std::shared_ptr<const MapData> MapDataConstPtr;

class MapLoader;
typedef std::shared_ptr<MapLoader> MapLoaderPtr;

class LongRand;
typedef std::shared_ptr<LongRand> LongRandPtr;

class Vfs;
typedef std::shared_ptr<Vfs> VfsPtr;

typedef unsigned short EntityId;

struct Difficulty
{
	enum : unsigned int
	{
		Easy= 1u,
		Normal= 2u,
		Hard= 4u,
		Deathmatch= 8u,
	};
};

typedef decltype(Difficulty::Easy) DifficultyType;

namespace Sound
{

class SoundEngine;
typedef std::shared_ptr<SoundEngine> SoundEnginePtr;

} // namespace Sound

} // namespace PanzerChasm
