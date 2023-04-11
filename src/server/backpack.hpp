#pragma once
#include <memory>

#include <vec.hpp>

#include "../game_constants.hpp"

namespace PanzerChasm
{

struct Backpack final
{
	m_Vec3 pos;
	float vertical_speed;
	float min_z;

	// Backpack content
	bool weapon[ GameConstants::weapon_count ]= { false };
	unsigned char ammo[ GameConstants::weapon_count ]= { 0 };

	bool red_key  = false;
	bool green_key= false;
	bool blue_key = false;

	unsigned char armor= 0;
};

typedef std::unique_ptr<Backpack> BackpackPtr;
typedef std::unique_ptr<const Backpack> BackpackConstPtr;

} // namespace PanzerChasm
