#pragma once

#include <glsl_program.hpp>
#include <polygon_buffer.hpp>
#include <texture.hpp>

#include "../drawers.hpp"
#include "../game_resources.hpp"
#include "../map_loader.hpp"
#include "../rendering_context.hpp"
#include "../time.hpp"

namespace PanzerChasm
{

class HudDrawer final
{
public:
	HudDrawer(
		const GameResourcesConstPtr& game_resources,
		const RenderingContext& rendering_context,
		const DrawersPtr& drawers );
	~HudDrawer();

	void AddMessage( const MapData::Message& message, Time current_time );
	void DrawCrosshair( unsigned int scale= 1u );
	void DrawCurrentMessage( unsigned int scale, Time current_time );

private:
	struct Vertex
	{
		short xy[2];
		short tex_coord[2];
	};

private:
	const GameResourcesConstPtr game_resources_;
	const Size2 viewport_size_;
	const DrawersPtr drawers_;

	r_GLSLProgram hud_shader_;
	r_PolygonBuffer quad_buffer_;

	r_Texture crosshair_texture_;

	MapData::Message current_message_;
	Time current_message_time_= Time::FromSeconds(0);
};

} // namespace PanzerChasm
