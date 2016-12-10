#pragma once
#include <memory>

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

class Vfs;
typedef std::shared_ptr<Vfs> VfsPtr;

} // namespace PanzerChasm
