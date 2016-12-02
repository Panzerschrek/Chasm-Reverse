#pragma once
#include <memory>

namespace PanzerChasm
{

class Map;

class MonsterBase;
typedef std::shared_ptr<MonsterBase> MonsterBasePtr;

class Monster;
typedef std::shared_ptr<Monster> MonsterPtr;

class Player;
typedef std::shared_ptr<Player> PlayerPtr;

class Server;

} // namespace PanzerChasm
