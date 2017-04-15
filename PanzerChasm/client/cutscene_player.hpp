#pragma once
#include "../fwd.hpp"
#include "../system_event.hpp"
#include "../time.hpp"
#include "cutscene_script.hpp"
#include "fwd.hpp"

namespace PanzerChasm
{

class CutscenePlayer final
{
public:
	CutscenePlayer(
		const GameResourcesConstPtr& game_resoruces,
		MapLoader& map_loader,
		IMapDrawer& map_drawer,
		MovementController& movement_controller,
		unsigned int map_number );
	~CutscenePlayer();

	void Process( const SystemEvents& events );
	void Draw();

	bool IsFinished() const;

private:
	struct CharacterState
	{
		unsigned int current_animation_number;
		Time current_animation_start_time= Time::FromSeconds(0);
	};

private:
	IMapDrawer& map_drawer_;
	MovementController& camera_controller_;
	MapDataConstPtr cutscene_map_data_;
	std::unique_ptr<MapState> map_state_;
	CutsceneScriptConstPtr script_;

	unsigned int first_character_static_model_index_;
	std::vector<CharacterState> characters_;
	unsigned int next_action_index_= 0u;

	m_Vec3 room_pos_;
	float room_angle_;

	m_Vec3 camera_pos_;
	m_Vec3 camara_angles_;

	Time current_countinuous_command_start_time_= Time::FromSeconds(0); // Zero if no active action.
};

} // namespace PanzerChasm
