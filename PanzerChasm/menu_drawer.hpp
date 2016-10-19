#pragma once

#include <glsl_program.hpp>
#include <polygon_buffer.hpp>
#include <texture.hpp>

#include "game_resources.hpp"

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
		unsigned int viewport_width,
		unsigned int viewport_height,
		const GameResources& game_resources );

	~MenuDrawer();

	unsigned int GetPictureWidth ( MenuPicture picture ) const;
	unsigned int GetPictureHeight( MenuPicture picture ) const;

	void DrawMenuBackground( unsigned int width, unsigned int height, unsigned int scale );

	void DrawMenuPicture(
		MenuPicture picture,
		const PictureColor* rows_colors,
		unsigned int scale );

private:
	struct Vertex
	{
		short xy[2];
		short tex_coord[2];
	};

private:
	unsigned int viewport_size_[2];

	r_GLSLProgram menu_background_shader_;
	r_Texture tiles_texture_;

	r_GLSLProgram menu_picture_shader_;
	r_Texture menu_pictures_[ size_t(MenuPicture::PicturesCount) ];

	r_PolygonBuffer polygon_buffer_;

};

} // namespace PanzerChasm
