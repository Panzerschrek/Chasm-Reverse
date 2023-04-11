#pragma once

#include "fwd.hpp"

namespace PanzerChasm
{

struct SharedDrawers
{
	explicit SharedDrawers( IDrawersFactory& drawers_factory );
	~SharedDrawers();

	// You should call VidClear before VidRestart.
	void VidClear();
	void VidRestart( IDrawersFactory& drawers_factory );

	IMenuDrawerPtr menu;
	ITextDrawerPtr text;
};

} // namespace PanzerChasm
