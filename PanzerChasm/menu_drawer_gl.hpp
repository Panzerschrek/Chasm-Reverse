#pragma once

#include <glsl_program.hpp>
#include <polygon_buffer.hpp>
#include <texture.hpp>

#include "fwd.hpp"
#include "i_menu_drawer.hpp"
#include "rendering_context.hpp"

namespace PanzerChasm
{

class MenuDrawerGL final : public IMenuDrawer
{
public:
	MenuDrawerGL(
		const RenderingContextGL& rendering_context,
		const GameResources& game_resources );

	virtual ~MenuDrawerGL();

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
	struct Vertex
	{
		short xy[2];
		short tex_coord[2];
	};

private:
	const Size2 viewport_size_;
	const unsigned int menu_scale_;
	const unsigned int console_scale_;

	r_GLSLProgram menu_background_shader_;
	r_Texture tiles_texture_;
	r_Texture loading_texture_;
	r_Texture pause_texture_;
	r_Texture game_background_texture_;

	r_GLSLProgram menu_picture_shader_;
	r_Texture menu_pictures_[ size_t(MenuPicture::PicturesCount) ];

	r_Texture framing_texture_;
	short framing_tex_coords_[4][2];

	r_Texture console_background_texture_;

	r_PolygonBuffer polygon_buffer_;

};

} // namespace PanzerChasm
