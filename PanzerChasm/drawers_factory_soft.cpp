#include "assert.hpp"
#include "client/hud_drawer_soft.hpp"
#include "client/map_drawer_soft.hpp"
#include "client/minimap_drawer_soft.hpp"
#include "menu_drawer_soft.hpp"
#include "text_drawer_soft.hpp"

#include "drawers_factory_soft.hpp"

namespace PanzerChasm
{

DrawersFactorySoft::DrawersFactorySoft(
	Settings& settings,
	const GameResourcesConstPtr& game_resources,
	const RenderingContextSoft& rendering_context )
	: settings_( settings )
	, game_resources_( game_resources )
	, rendering_context_( rendering_context )
{
	PC_ASSERT( game_resources_ != nullptr );
}

DrawersFactorySoft::~DrawersFactorySoft()
{}

ITextDrawerPtr DrawersFactorySoft::CreateTextDrawer()
{
	return ITextDrawerPtr( new TextDrawerSoft( rendering_context_, *game_resources_ ) );
}

IMenuDrawerPtr DrawersFactorySoft::CreateMenuDrawer()
{
	return IMenuDrawerPtr( new MenuDrawerSoft( rendering_context_, *game_resources_ ) );
}

IHudDrawerPtr DrawersFactorySoft::CreateHUDDrawer( const SharedDrawersPtr& shared_drawers )
{
	PC_ASSERT( shared_drawers != nullptr );

	return
		IHudDrawerPtr(
			new HudDrawerSoft(
				game_resources_,
				rendering_context_,
				shared_drawers ) );
}

IMapDrawerPtr DrawersFactorySoft::CreateMapDrawer()
{
	return
		IMapDrawerPtr(
			new MapDrawerSoft(
				settings_,
				game_resources_,
				rendering_context_ ) );
}

IMinimapDrawerPtr DrawersFactorySoft::CreateMinimapDrawer()
{
	return
		IMinimapDrawerPtr(
			new MinimapDrawerSoft(
				settings_,
				game_resources_,
				rendering_context_ ) );
}

} // namespace PanzerChasm
