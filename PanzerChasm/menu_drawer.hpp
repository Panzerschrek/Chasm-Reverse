#pragma once

#include <glsl_program.hpp>
#include <polygon_buffer.hpp>
#include <texture.hpp>

#include "game_resources.hpp"
#include "rendering_context.hpp"

namespace PanzerChasm
{

class MenuDrawer final
{
public:
	enum class MenuPicture
	{
		Main,
		New,
		Network,

		PicturesCount
	};

	enum class PictureColor
	{
		Unactive,
		Active,
		Disabled
	};

	MenuDrawer(
		const RenderingContext& rendering_context,
		const GameResources& game_resources );

	~MenuDrawer();

	Size2 GetViewportSize() const;
	Size2 GetPictureSize( MenuPicture picture ) const;

	void DrawMenuBackground(
		int x, int y,
		unsigned int width, unsigned int height,
		unsigned int scale );

	void DrawMenuPicture(
		int x, int y,
		MenuPicture picture,
		const PictureColor* rows_colors,
		unsigned int scale );

	void DrawConsoleBackground( float console_pos );

private:
	struct Vertex
	{
		short xy[2];
		short tex_coord[2];
	};

private:
	const Size2 viewport_size_;

	r_GLSLProgram menu_background_shader_;
	r_Texture tiles_texture_;

	r_GLSLProgram menu_picture_shader_;
	r_Texture menu_pictures_[ size_t(MenuPicture::PicturesCount) ];

	r_Texture framing_texture_;
	short framing_tex_coords_[4][2];

	r_Texture console_background_texture_;

	r_PolygonBuffer polygon_buffer_;

};

} // namespace PanzerChasm
