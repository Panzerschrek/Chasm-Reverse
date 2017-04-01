#pragma once

#include <glsl_program.hpp>
#include <polygon_buffer.hpp>
#include <texture.hpp>

#include "../rendering_context.hpp"
#include "hud_drawer_base.hpp"

namespace PanzerChasm
{

class HudDrawerGL final : public HudDrawerBase
{
public:
	HudDrawerGL(
		const GameResourcesConstPtr& game_resources,
		const RenderingContextGL& rendering_context,
		const SharedDrawersPtr& shared_drawers );
	virtual ~HudDrawerGL() override;

	virtual void DrawCrosshair() override;
	virtual void DrawHud( bool draw_second_hud, const char* map_name ) override;

private:
	struct Vertex
	{
		short xy[2];
		short tex_coord[2];
	};

private:
	void LoadTexture( const char* file_name, unsigned char alpha_color_index, r_Texture& out_texture );

private:
	const Size2 viewport_size_;

	r_GLSLProgram hud_shader_;
	r_PolygonBuffer quad_buffer_;

	r_Texture crosshair_texture_;
	r_Texture weapon_icons_texture_;
	r_Texture hud_numbers_texture_;
	r_Texture hud_background_texture_;
};

} // namespace PanzerChasm
