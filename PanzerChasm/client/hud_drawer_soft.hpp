#pragma once

#include "../rendering_context.hpp"
#include "i_hud_drawer.hpp"

namespace PanzerChasm
{

class HudDrawerSoft final : public IHudDrawer
{
public:
	HudDrawerSoft(
		const GameResourcesConstPtr& game_resources,
		const RenderingContextSoft& rendering_context,
		const SharedDrawersPtr& shared_drawers );
	virtual ~HudDrawerSoft() override;

	virtual void AddMessage( const MapData::Message& message, Time current_time ) override;
	virtual void ResetMessage() override;
	virtual void SetPlayerState( const Messages::PlayerState& player_state, unsigned int current_weapon_number ) override;

	virtual void DrawCrosshair() override;
	virtual void DrawCurrentMessage( Time current_time ) override;
	virtual void DrawHud( bool draw_second_hud, const char* map_name ) override;

private:
};

} // namespace PanzerChasm
