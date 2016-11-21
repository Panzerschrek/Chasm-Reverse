#pragma once
#include "../game_constants.hpp"

namespace PanzerChasm
{

// Codes for game items.
// Based on info from files CHASM.INF and RESOURCE.XX.
enum class ACode : unsigned char
{
	Weapon_First= 1u,
	Weapon_Last= Weapon_First + GameConstants::weapon_count - 1u,

	// Ammo
	// For ammo major decimal digit is weapon number, minor decimal digit is count of ammo portion.
	// Portion size - see weapons description.
	Ammo_First= Weapon_First * 10u,
	Ammo_Last= Weapon_Last * 10u + 9u,

	// Useful items
	Item_chojin= 133,
	Item_Shield= 132,
	Item_Invisibility= 131,
	Item_Armor= 128,
	Item_Helmet= 124,
	Item_Life= 102,
	Item_BigLife= 120,

	// Keys
	RedKey= 141,
	GreenKey= 142,
	BlueKey= 143,

	Switch= 255u,
};

} // PanzerChasm
