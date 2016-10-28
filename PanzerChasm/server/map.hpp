#pragma once

#include "../map_loader.hpp"
#include "../messages_sender.hpp"

namespace PanzerChasm
{

// Server-side map logic here.
class Map final
{
public:
	typedef float TimePoint;
	typedef decltype(TimePoint() - TimePoint()) TimeInterval;

	explicit Map( const MapDataConstPtr& map_data );
	~Map();

	void ProcessPlayerPosition( TimePoint current_time, const m_Vec3& pos, MessagesSender& messages_sender );
	void Tick( TimePoint current_time, TimeInterval frame_delta );

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
		TimePoint last_state_change_time= 0;

		TimePoint last_change_time= 0;
	};

	struct DynamicWall
	{
		m_Vec2 vert_pos[2];
		float z;
	};

	typedef std::vector<DynamicWall> DynamicWalls;

private:
	const MapDataConstPtr map_data_;
	DynamicWalls dynamic_walls_;

	std::vector<ProcedureState> procedures_;
};

} // PanzerChasm
