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
typedef std::shared_ptr<const Player> PlayerConstPtr;
typedef std::weak_ptr<Player> PlayerWeakPtr;
typedef std::weak_ptr<const Player> PlayerConstWeakPtr;

class LongRand;
typedef std::shared_ptr<LongRand> LongRandPtr;

class Server;

} // namespace PanzerChasm
