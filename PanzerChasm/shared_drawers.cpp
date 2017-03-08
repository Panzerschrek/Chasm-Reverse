#include "assert.hpp"
#include "i_drawers_factory.hpp"
#include "i_menu_drawer.hpp"
#include "i_text_drawer.hpp"

#include "shared_drawers.hpp"

namespace PanzerChasm
{

SharedDrawers::SharedDrawers( IDrawersFactory& drawers_factory )
	: menu( drawers_factory.CreateMenuDrawer() )
	, text( drawers_factory.CreateTextDrawer() )
{
	PC_ASSERT( menu != nullptr );
	PC_ASSERT( text != nullptr );
}

SharedDrawers::~SharedDrawers()
{}

} // namespace PanzerChasm
