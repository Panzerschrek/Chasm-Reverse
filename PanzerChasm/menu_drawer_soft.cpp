#include "assert.hpp"

#include "menu_drawer_soft.hpp"

namespace PanzerChasm
{

MenuDrawerSoft::MenuDrawerSoft(
	const RenderingContextSoft& rendering_context,
	const GameResources& game_resources )
	: viewport_size_( rendering_context.viewport_size )
{
	// TODO
	PC_UNUSED( game_resources );
}

MenuDrawerSoft::~MenuDrawerSoft()
{}

Size2 MenuDrawerSoft::GetViewportSize() const
{
	return viewport_size_;
}

unsigned int MenuDrawerSoft::GetMenuScale() const
{
	// TODO
	return 1u;
}

unsigned int MenuDrawerSoft::GetConsoleScale() const
{
	// TODO
	return 1u;
}

Size2 MenuDrawerSoft::GetPictureSize( const MenuPicture picture ) const
{
	// TODO
	PC_UNUSED( picture );
	return Size2( 1u, 1u );
}

void MenuDrawerSoft::DrawMenuBackground(
	const int x, const int y,
	const unsigned int width, const unsigned int height )
{
	// TODO
	PC_UNUSED( x );
	PC_UNUSED( y );
	PC_UNUSED( width );
	PC_UNUSED( height );
}

void MenuDrawerSoft::DrawMenuPicture(
	const int x, const int y,
	const MenuPicture picture,
	const PictureColor* const rows_colors )
{
	// TODO
	PC_UNUSED( x );
	PC_UNUSED( y );
	PC_UNUSED( picture );
	PC_UNUSED( rows_colors );
}

void MenuDrawerSoft::DrawConsoleBackground( const float console_pos )
{
	// TODO
	PC_UNUSED( console_pos );
}

void MenuDrawerSoft::DrawLoading( const float progress )
{
	// TODO
	PC_UNUSED( progress );
}

void MenuDrawerSoft::DrawPaused()
{
	// TODO
}

void MenuDrawerSoft::DrawGameBackground()
{
	// TODO
}

} // namespace PanzerChas
