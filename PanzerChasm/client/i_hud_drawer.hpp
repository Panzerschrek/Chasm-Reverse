#pragma once
#include "../map_loader.hpp"
#include "../messages.hpp"
#include "../time.hpp"

namespace PanzerChasm
{

class IHudDrawer
{
public:
	virtual ~IHudDrawer(){}

	virtual void AddMessage( const MapData::Message& message, Time current_time )= 0;
	virtual void ResetMessage()= 0;
	virtual void SetPlayerState( const Messages::PlayerState& player_state, unsigned int current_weapon_number )= 0;

	virtual void DrawCrosshair()= 0;
	virtual void DrawCurrentMessage( Time current_time )= 0;
	virtual void DrawHud( bool draw_second_hud, const char* map_name )= 0;
};

} // namespace PanzerChasm
