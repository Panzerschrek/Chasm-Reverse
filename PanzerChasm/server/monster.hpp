#pragma once
#include <memory>

#include <vec.hpp>

#include "../map_loader.hpp"

namespace PanzerChasm
{

class Monster final
{
public:
	explicit Monster( const MapData::Monster& map_monster );
	~Monster();

	unsigned char MonsterId() const;
	const m_Vec3& Position() const;
	float Angle() const;

private:
	const unsigned char monster_id_;
	m_Vec3 pos_;
	float angle_;
};

typedef std::unique_ptr<Monster> MonsterPtr;

} // namespace PanzerChasm
