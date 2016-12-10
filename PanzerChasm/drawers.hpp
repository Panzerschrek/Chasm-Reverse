#pragma once
#include <memory>

#include <text_draw.hpp>
#include <menu_drawer.hpp>

namespace PanzerChasm
{

struct Drawers
{
	Drawers(
		const RenderingContext& rendering_context,
		const GameResources& game_resources );
	~Drawers();

	MenuDrawer menu;
	TextDraw text;
};

} // namespace PanzerChasm
