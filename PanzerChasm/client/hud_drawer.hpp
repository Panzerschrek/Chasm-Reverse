#pragma once

#include <glsl_program.hpp>
#include <polygon_buffer.hpp>
#include <texture.hpp>

#include "../map_loader.hpp"
#include "../messages.hpp"
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
	void ResetMessage();
	void SetPlayerState( const Messages::PlayerState& player_state, unsigned int current_weapon_number );

	void DrawCrosshair();
	void DrawCurrentMessage( Time current_time );
	void DrawHud( bool draw_second_hud );

private:
	struct Vertex
	{
		short xy[2];
		short tex_coord[2];
	};

private:
	void LoadTexture( const char* file_name, unsigned char alpha_color_index, r_Texture& out_texture );

private:
	const GameResourcesConstPtr game_resources_;
	const Size2 viewport_size_;
	const DrawersPtr drawers_;
	const unsigned int scale_;

	r_GLSLProgram hud_shader_;
	r_PolygonBuffer quad_buffer_;

	r_Texture crosshair_texture_;
	r_Texture weapon_icons_texture_;
	r_Texture hud_numbers_texture_;
	r_Texture hud_background_texture_;

	// State

	MapData::Message current_message_;
	Time current_message_time_= Time::FromSeconds(0); // 0 means no active message.

	unsigned int current_weapon_number_= 0u;
	Messages::PlayerState player_state_;
};

} // namespace PanzerChasm
