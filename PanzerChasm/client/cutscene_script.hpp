#pragma once
#include "../map_loader.hpp"

namespace PanzerChasm
{

struct CutsceneScript
{
public:
	static constexpr unsigned int c_max_character_animations= 8u;

	struct Character
	{
		char model_file_name[ MapData::c_max_file_name_size ];
		m_Vec3 pos;
		float angle;

		char idle_animation_file_name[ MapData::c_max_file_name_size ];
		char animations_file_name[ c_max_character_animations ][ MapData::c_max_file_name_size ];
	};

	struct ActionCommand
	{
		static const unsigned int c_max_params= 6u;

		enum class Type
		{
			None,
			Delay,
			Say,
			Voice,
			Setani,
			WaitKey,
			MoveCamTo,
		};

		Type type;
		std::string params[6];
	};

public:
	unsigned int ambient_sound_number;
	unsigned int room_number= 0u;

	struct
	{
		m_Vec3 pos;
		float angle;
	} camera_params;

	std::vector<Character> characters;
	std::vector<ActionCommand> commands;
};

typedef std::shared_ptr<CutsceneScript> CutsceneScriptPtr;
typedef std::shared_ptr<const CutsceneScript> CutsceneScriptConstPtr;

CutsceneScriptPtr LoadCutsceneScript( const Vfs& vfs, unsigned int map_number );

} // namespace PanzerChasm
