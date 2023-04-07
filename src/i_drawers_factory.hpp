#pragma once
#include <memory>

#include "fwd.hpp"

namespace PanzerChasm
{

class IDrawersFactory
{
public:
	virtual ~IDrawersFactory(){}

	virtual ITextDrawerPtr CreateTextDrawer()= 0;
	virtual IMenuDrawerPtr CreateMenuDrawer()= 0;
	virtual IHudDrawerPtr CreateHUDDrawer( const SharedDrawersPtr& shared_drawers )= 0;
	virtual IMapDrawerPtr CreateMapDrawer()= 0;
	virtual IMinimapDrawerPtr CreateMinimapDrawer()= 0;
};

} // namespace PanzerChasm
