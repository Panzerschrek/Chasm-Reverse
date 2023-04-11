#pragma once
#include "../map_loader.hpp"
#include "../messages.hpp"
#include "../time.hpp"
#include "../game_constants.hpp"

namespace PanzerChasm
{

class IHudDrawer
{
public:
	struct NetgameScores
	{
		unsigned int scores[ GameConstants::max_players ];
		unsigned int score_count;
		unsigned int active_score_number;
	};

public:
	virtual ~IHudDrawer(){}

	virtual void AddMessage( const MapData::Message& message, Time current_time )= 0;
	virtual void ResetMessage()= 0;
	virtual void SetPlayerState( const Messages::PlayerState& player_state, unsigned int current_weapon_number )= 0;

	virtual void DrawCrosshair()= 0;
	virtual void DrawCurrentMessage( Time current_time )= 0;
	virtual void DrawHud(
		bool draw_second_hud,
		const char* map_name,
		const NetgameScores* netgame_scores )= 0;
	virtual void DrawSmallHud()= 0;
};

} // namespace PanzerChasm
