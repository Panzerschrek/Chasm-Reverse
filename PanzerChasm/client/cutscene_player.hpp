#pragma once
#include <matrix.hpp>

#include "../fwd.hpp"
#include "../system_event.hpp"
#include "../time.hpp"
#include "cutscene_script.hpp"
#include "fwd.hpp"
#include "i_map_drawer.hpp"

namespace PanzerChasm
{

class CutscenePlayer final
{
public:
	CutscenePlayer(
		const GameResourcesConstPtr& game_resoruces,
		MapLoader& map_loader,
		const Sound::SoundEnginePtr& sound_engine,
		const SharedDrawersPtr& shared_drawers,
		IMapDrawer& map_drawer,
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

	struct Camera
	{
		m_Vec3 pos;
		float angle_z;
	};

private:
	const Sound::SoundEnginePtr sound_engine_;
	const SharedDrawersPtr shared_drawers_;
	IMapDrawer& map_drawer_;
	MapDataConstPtr cutscene_map_data_;
	std::unique_ptr<MapState> map_state_;
	CutsceneScriptConstPtr script_;

	unsigned int first_character_model_index_;
	std::vector<CharacterState> characters_;
	std::vector<IMapDrawer::MapRelatedModel> characters_map_models_;

	unsigned int next_action_index_= 0u;

	m_Vec3 room_pos_;
	m_Mat4 room_rotation_matrix_;
	float room_angle_;

	Camera camera_previous_, camera_next_, camera_current_;

	Time current_countinuous_command_start_time_= Time::FromSeconds(0); // Zero if no active action.

	// Say text.
	static constexpr unsigned int c_say_lines= 5u;
	std::string say_lines_[ c_say_lines ];
	unsigned int next_say_line_= 0u;
};

} // namespace PanzerChasm
