#include "monster.hpp"

namespace PanzerChasm
{

Monster::Monster( const MapData::Monster& map_monster )
	: monster_id_( map_monster.monster_id )
	, pos_( map_monster.pos, 0.0f )
	, angle_( map_monster.angle )
{}

Monster::~Monster()
{}

unsigned char Monster::MonsterId() const
{
	return monster_id_;
}

const m_Vec3& Monster::Position() const
{
	return pos_;
}

float Monster::Angle() const
{
	return angle_;
}

} // namespace PanzerChasm
