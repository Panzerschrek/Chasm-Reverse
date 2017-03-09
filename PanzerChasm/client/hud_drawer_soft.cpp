#include "../assert.hpp"

#include "hud_drawer_soft.hpp"

namespace PanzerChasm
{

HudDrawerSoft::HudDrawerSoft(
	const GameResourcesConstPtr& game_resources,
	const RenderingContextSoft& rendering_context,
	const SharedDrawersPtr& shared_drawers )
{
	PC_UNUSED( game_resources );
	PC_UNUSED( rendering_context );
	PC_UNUSED( shared_drawers );
}

HudDrawerSoft::~HudDrawerSoft()
{}

void HudDrawerSoft::AddMessage( const MapData::Message& message, const Time current_time )
{
	// TODO
	PC_UNUSED( message );
	PC_UNUSED( current_time );
}

void HudDrawerSoft::ResetMessage()
{
	// TODO
}

void HudDrawerSoft::SetPlayerState( const Messages::PlayerState& player_state, const unsigned int current_weapon_number )
{
	// TODO
	PC_UNUSED( player_state );
	PC_UNUSED( current_weapon_number );
}

void HudDrawerSoft::DrawCrosshair()
{
	// TODO
}

void HudDrawerSoft::DrawCurrentMessage( const Time current_time )
{
	// TODO
	PC_UNUSED( current_time );
}

void HudDrawerSoft::DrawHud( const bool draw_second_hud, const char* const map_name )
{
	// TODO
	PC_UNUSED( draw_second_hud );
	PC_UNUSED( map_name );
}

} // namespace PanzerChasm
