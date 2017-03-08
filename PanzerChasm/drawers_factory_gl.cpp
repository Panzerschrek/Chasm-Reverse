#include "client/hud_drawer.hpp"
#include "client/map_drawer.hpp"
#include "client/minimap_drawer.hpp"
#include "menu_drawer.hpp"
#include "text_draw.hpp"

#include "drawers_factory_gl.hpp"

namespace PanzerChasm
{

DrawersFactoryGL::DrawersFactoryGL(
	Settings& settings,
	const GameResourcesConstPtr& game_resources,
	const RenderingContext& rendering_context )
	: settings_(settings)
	, game_resources_(game_resources)
	, rendering_context_(rendering_context)
{
	PC_ASSERT( game_resources_ != nullptr );
}

DrawersFactoryGL::~DrawersFactoryGL()
{}

ITextDrawerPtr DrawersFactoryGL::CreateTextDrawer()
{
	return ITextDrawerPtr( new TextDraw( rendering_context_, *game_resources_ ) );
}

IMenuDrawerPtr DrawersFactoryGL::CreateMenuDrawer()
{
	return IMenuDrawerPtr( new MenuDrawer( rendering_context_, *game_resources_ ) );
}

IHudDrawerPtr DrawersFactoryGL::CreateHUDDrawer( const SharedDrawersPtr& shared_drawers )
{
	PC_ASSERT( shared_drawers != nullptr );

	return
		IHudDrawerPtr(
			new HudDrawer(
				game_resources_,
				rendering_context_,
				shared_drawers ) );
}

IMapDrawerPtr DrawersFactoryGL::CreateMapDrawer()
{
	return
		IMapDrawerPtr(
			new MapDrawer(
				settings_,
				game_resources_,
				rendering_context_ ) );
}

IMinimapDrawerPtr DrawersFactoryGL::CreateMinimapDrawer()
{
	return
		IMinimapDrawerPtr(
			new MinimapDrawer(
				settings_,
				game_resources_,
				rendering_context_ ) );
}

} // namespace PanzerChasm
