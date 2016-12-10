#pragma once
#include <memory>

namespace PanzerChasm
{

struct GameResources;
typedef std::shared_ptr<GameResources> GameResourcesPtr;
typedef std::shared_ptr<const GameResources> GameResourcesConstPtr;

} // namespace PanzerChasm
