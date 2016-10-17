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

	MenuDrawer(
		unsigned int viewport_width,
		unsigned int viewport_height,
		const GameResources& game_resources );

	~MenuDrawer();

	void DrawMenuBackground( unsigned int width, unsigned int height, unsigned int texture_scale );

private:
	struct Vertex
	{
		short xy[2];
	};

private:
	unsigned int viewport_size_[2];

	r_GLSLProgram shader_;
	r_Texture tiles_texture_;

	r_PolygonBuffer polygon_buffer_;

	r_Texture menu_pictures_[ size_t(MenuPicture::PicturesCount) ];
};

} // namespace PanzerChasm
