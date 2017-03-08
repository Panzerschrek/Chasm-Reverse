#pragma once

#include "fwd.hpp"

namespace PanzerChasm
{

struct SharedDrawers
{
	explicit SharedDrawers( IDrawersFactory& drawers_factory );
	~SharedDrawers();

	IMenuDrawerPtr menu;
	ITextDrawerPtr text;
};

} // namespace PanzerChasm
