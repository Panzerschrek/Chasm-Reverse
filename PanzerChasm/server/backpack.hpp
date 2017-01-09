#pragma once
#include <memory>

#include <vec.hpp>

#include "../game_constants.hpp"

namespace PanzerChasm
{

struct Backpack final
{
	m_Vec3 pos;
	float angle;

	// Backpack content
	bool weapon[ GameConstants::weapon_count ];
	unsigned char ammo[ GameConstants::weapon_count ];

	bool red_key  = false;
	bool green_key= false;
	bool blue_key = false;

	unsigned char armor;
};

typedef std::unique_ptr<Backpack> BackpackPtr;
typedef std::unique_ptr<const Backpack> BackpackConstPtr;

} // namespace PanzerChasm
