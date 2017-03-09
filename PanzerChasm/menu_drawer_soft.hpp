#pragma once

#include "fwd.hpp"
#include "i_menu_drawer.hpp"
#include "rendering_context.hpp"

namespace PanzerChasm
{

class MenuDrawerSoft final : public IMenuDrawer
{
public:
	MenuDrawerSoft(
		const RenderingContextSoft& rendering_context,
		const GameResources& game_resources );

	virtual ~MenuDrawerSoft();

	virtual Size2 GetViewportSize() const override;
	virtual unsigned int GetMenuScale() const override;
	virtual unsigned int GetConsoleScale() const override;

	virtual Size2 GetPictureSize( MenuPicture picture ) const override;

	virtual void DrawMenuBackground(
		int x, int y,
		unsigned int width, unsigned int height ) override;

	virtual void DrawMenuPicture(
		int x, int y,
		MenuPicture picture,
		const PictureColor* rows_colors ) override;

	virtual void DrawConsoleBackground( float console_pos ) override;
	virtual void DrawLoading( float progress ) override;
	virtual void DrawPaused() override;
	virtual void DrawGameBackground() override;

private:
	const Size2 viewport_size_;
};

} // namespace PanzerChasm
