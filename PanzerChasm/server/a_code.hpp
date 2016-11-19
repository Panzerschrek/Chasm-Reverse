#pragma once

namespace PanzerChasm
{

// Codes for game items.
// Based on info from files CHASM.INF and RESOURCE.XX.
enum class ACode : unsigned char
{
	// Weapon
	Weapon_Rifle= 2,
	Weapon_DoubleBarreldGun= 3,
	Weapon_Disk= 4,
	Weapon_Arbalet= 5,
	Weapon_GrenadeLauncher= 6,
	Weapon_MD= 8,

	// Ammo
	Ammo_Bullets= 21,
	Ammo_gbbull= 22,
	Ammo_vbox= 35,
	Ammo_Saws= 41,
	Ammo_Arrows= 52,
	Ammo_Grenades= 62,
	Ammo_Mines= 74,
	Ammo_MDBox= 82,

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
