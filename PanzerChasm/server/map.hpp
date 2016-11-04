#pragma once

#include "../map_loader.hpp"
#include "../messages_sender.hpp"
#include "../time.hpp"
#include "player.hpp"

namespace PanzerChasm
{

// Server-side map logic here.
class Map final
{
public:
	Map( const MapDataConstPtr& map_data, Time map_start_time );
	~Map();

	void ProcessPlayerPosition( Time current_time, Player& player, MessagesSender& messages_sender );
	void Tick( Time current_time );

	void SendUpdateMessages( MessagesSender& messages_sender ) const;

private:
	struct ProcedureState
	{
		enum class MovementState
		{
			None,
			Movement,
			BackWait,
			ReverseMovement,
		};

		bool alive= true;
		bool locked= false;
		bool first_message_printed= false;

		MovementState movement_state= MovementState::None;
		unsigned int movement_loop_iteration= 0u;
		float movement_stage= 0.0f; // stage of current movement state [0; 1]
		Time last_state_change_time= Time::FromSeconds(0);

		Time last_change_time= Time::FromSeconds(0);
	};

	struct DynamicWall
	{
		m_Vec2 vert_pos[2];
		float z;
	};

	typedef std::vector<DynamicWall> DynamicWalls;

	struct StaticModel
	{
		m_Vec3 pos;
		float angle;

		unsigned char model_id;
		bool destroyed= false;

		enum class AnimationState
		{
			Animation,
			SingleFrame,
		};

		AnimationState animation_state;
		Time animation_start_time= Time::FromSeconds(0);
		unsigned int animation_start_frame;

		unsigned int current_animation_frame;
	};

	typedef std::vector<StaticModel> StaticModels;

private:
	void TryActivateProcedure( unsigned int procedure_number, const Time current_time, Player& player, MessagesSender& messages_sender );

private:
	const MapDataConstPtr map_data_;
	DynamicWalls dynamic_walls_;

	std::vector<ProcedureState> procedures_;

	StaticModels static_models_;
};

} // PanzerChasm
