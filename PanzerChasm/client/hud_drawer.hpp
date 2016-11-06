#pragma once

#include <glsl_program.hpp>
#include <polygon_buffer.hpp>
#include <texture.hpp>

#include "../game_resources.hpp"
#include "../rendering_context.hpp"

namespace PanzerChasm
{

class HudDrawer final
{
public:
	HudDrawer(
		const GameResourcesConstPtr& game_resources,
		const RenderingContext& rendering_context );
	~HudDrawer();

	void DrawCrosshair( unsigned int scale= 1u );

private:
	struct Vertex
	{
		short xy[2];
		short tex_coord[2];
	};

private:
	const GameResourcesConstPtr game_resources_;
	const Size2 viewport_size_;

	r_GLSLProgram hud_shader_;
	r_PolygonBuffer quad_buffer_;

	r_Texture crosshair_texture_;
};

} // namespace PanzerChasm
