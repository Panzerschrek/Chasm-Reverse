#pragma once

namespace PanzerChasm
{

namespace Sound
{

struct SoundId
{
	enum : unsigned char
	{
		MenuScroll= 2u,
		MenuOn= 3u,
		MenuChange= 4u,
		MenuSelect= 5u,
		FirstWeaponFire= 21u,
		FirstWeaponPickup=  51u,

		ItemUp= 60u,
		Message= 61u,
		Health= 62u,
		GetKey= 63u,
		BreakGlass0= 68u,
		BreakGlass1= 69u,
		BreakGlass2= 70u,
	};
};

} // namespace Sound

} // namespace PanzerChasm
