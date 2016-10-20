#pragma once
#include <memory>

#include "game_resources.hpp"
#include "menu_drawer.hpp"
#include "rendering_context.hpp"
#include "system_event.hpp"
#include "text_draw.hpp"

namespace PanzerChasm
{

class MenuBase;

// Main game munu class.
class Menu final
{
public:
	Menu(
		const RenderingContext& rendering_context,
		const GameResourcesPtr& game_resources );
	~Menu();

	void ProcessEvents( const SystemEvents& events );
	void Draw();

private:
	TextDraw text_draw_;
	MenuDrawer menu_drawer_;

	std::unique_ptr<MenuBase> root_menu_;
	MenuBase* current_menu_= nullptr;
};

} // namespace PanzerChasm
