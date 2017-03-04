#pragma once
#include <functional>
#include <memory>
#include <vector>

// Forward declarations, basic typedefs here.

namespace PanzerChasm
{

constexpr unsigned int g_max_save_comment_size= 32u;
typedef std::array< char, g_max_save_comment_size > SaveComment;

typedef std::function< void( float progress, const char* caption ) > DrawLoadingCallback;

class IConnection;
typedef std::shared_ptr<IConnection> IConnectionPtr;

class IConnectionsListener;
typedef std::shared_ptr<IConnectionsListener> IConnectionsListenerPtr;

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

struct ObjSprite;

typedef std::vector<unsigned char> SaveLoadBuffer;
class SaveStream;
class LoadStream;

class Settings;

class Vfs;
typedef std::shared_ptr<Vfs> VfsPtr;

class SystemWindow;

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

enum class GameRules
{
	SinglePlayer,
	Cooperative,
	Deathmatch,
};

namespace Sound
{

class SoundEngine;
typedef std::shared_ptr<SoundEngine> SoundEnginePtr;

} // namespace Sound

} // namespace PanzerChasm
