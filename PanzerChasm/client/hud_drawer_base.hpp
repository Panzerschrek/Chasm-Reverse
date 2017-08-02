#pragma once

#include "../time.hpp"
#include "i_hud_drawer.hpp"

namespace PanzerChasm
{

class HudDrawerBase : public IHudDrawer
{
public:
	HudDrawerBase(
		const GameResourcesConstPtr& game_resources,
		const SharedDrawersPtr& shared_drawers );
	~HudDrawerBase();

public: // IHudDrawer
	virtual void AddMessage( const MapData::Message& message, Time current_time ) override;
	virtual void ResetMessage() override;
	virtual void SetPlayerState( const Messages::PlayerState& player_state, unsigned int current_weapon_number ) override;

	virtual void DrawCurrentMessage( Time current_time ) override;

	virtual void DrawSmallHud() override;

protected:
	// Shared constants.
	static constexpr unsigned int c_hud_line_height= 32u;
	static constexpr unsigned int c_net_hud_line_height= 12u;
	static constexpr unsigned int c_cross_offset= 4u;

	static constexpr unsigned int c_weapon_icon_x_offset= 105u;
	static constexpr unsigned int c_weapon_icon_y_offset= 4u;
	static constexpr unsigned int c_weapon_icon_border= 1u;

	static constexpr unsigned int c_health_x_offset= 104u;
	static constexpr unsigned int c_health_red_value= 25u;
	static constexpr unsigned int c_ammo_x_offset= 205u;
	static constexpr unsigned int c_ammo_red_value= 10u;
	static constexpr unsigned int c_armor_x_offset= 315u;
	static constexpr unsigned int c_armor_red_value= 10u;

	static constexpr unsigned int c_netgame_score_digit_width= 12u;
	static constexpr unsigned int c_netgame_score_digit_width_to_draw= 6u;
	static constexpr unsigned int c_netgame_score_number_x_offset= 30u;
	static constexpr unsigned int c_netgame_score_number_y_offset= 3u;

	static const char c_weapon_icons_image_file_name[];
	static const char c_hud_numbers_image_file_name[];
	static const char c_hud_background_image_file_name[];
	static const char c_netgame_score_numbers_image_file_name[];

protected:
	void CreateCrosshairTexture( unsigned int* out_size, std::vector<unsigned char>& out_data );
	void DrawKeysAndStat( unsigned int x_offset, const char* map_name );

private:
	static unsigned int CalculateHudScale( const Size2& viewport_size );

protected:
	const GameResourcesConstPtr game_resources_;
	const SharedDrawersPtr shared_drawers_;
	const unsigned int scale_;

	// State
	MapData::Message current_message_;
	Time current_message_time_= Time::FromSeconds(0); // 0 means no active message.

	unsigned int current_weapon_number_= 0u;
	Messages::PlayerState player_state_;
};

} // namespace PanzerChasm
