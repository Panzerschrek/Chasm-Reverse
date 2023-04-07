#pragma once
#include <array>
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

class MessagesSender;

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

// Draw stuff

class ITextDrawer;
typedef std::unique_ptr<ITextDrawer> ITextDrawerPtr;

class IMenuDrawer;
typedef std::unique_ptr<IMenuDrawer> IMenuDrawerPtr;

class IHudDrawer;
typedef std::unique_ptr<IHudDrawer> IHudDrawerPtr;

class IMapDrawer;
typedef std::unique_ptr<IMapDrawer> IMapDrawerPtr;

class IMinimapDrawer;
typedef std::unique_ptr<IMinimapDrawer> IMinimapDrawerPtr;

struct SharedDrawers;
typedef std::shared_ptr<const SharedDrawers> SharedDrawersPtr;

class IDrawersFactory;
typedef std::shared_ptr<IDrawersFactory> IDrawersFactoryPtr;

struct Difficulty
{
	enum : unsigned int
	{
		Easy= 1u,
		Normal= 2u,
		Hard= 4u,
	};
};

typedef decltype(Difficulty::Easy) DifficultyType;

// This enum is a part of net protocol. Change protocol version, if this changed.
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
