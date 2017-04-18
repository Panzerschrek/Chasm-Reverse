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
	virtual void DrawBriefBar() override;

private:
	struct Picture
	{
		unsigned int size[2];
		std::vector<unsigned char> data;
	};

private:
	const RenderingContextSoft rendering_context_;
	const unsigned int menu_scale_;
	const unsigned int console_scale_;

	Picture menu_pictures_[ size_t(MenuPicture::PicturesCount) ];

	Picture tiles_picture_;
	Picture loading_picture_;
	Picture game_background_picture_;
	Picture pause_picture_;

	Picture console_background_picture_;
};

} // namespace PanzerChasm
