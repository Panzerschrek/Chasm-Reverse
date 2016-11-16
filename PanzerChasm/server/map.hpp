#pragma once
#include <unordered_map>

#include "../map_loader.hpp"
#include "../messages_sender.hpp"
#include "../time.hpp"
#include "monster.hpp"
#include "player.hpp"

namespace PanzerChasm
{

// Server-side map logic here.
class Map final
{
public:
	Map(
		const MapDataConstPtr& map_data,
		const GameResourcesConstPtr& game_resources,
		Time map_start_time );
	~Map();

	void SpawnPlayer( Player& player );

	void Shoot(
		unsigned int rocket_id,
		const m_Vec3& from,
		const m_Vec3& normalized_direction,
		Time current_time );

	void ProcessPlayerPosition( Time current_time, Player& player, MessagesSender& messages_sender );
	void Tick( Time current_time );

	void SendMessagesForNewlyConnectedPlayer( MessagesSender& messages_sender ) const;
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

	struct Rocket
	{
		Rocket(
			unsigned int in_rocket_id,
			const m_Vec3& in_start_point,
			const m_Vec3& in_normalized_direction,
			const Time start_time );

		bool HasInfiniteSpeed( const GameResources& game_resources ) const;

		// Start parameters
		unsigned int rocket_id;
		m_Vec3 start_point;
		m_Vec3 normalized_direction;
		Time start_time;

		m_Vec3 previous_position;
		float track_length;
	};

	typedef std::vector<Rocket> Rockets;

	struct HitResult
	{
		enum class ObjectType
		{
			None, StaticWall, DynamicWall, Model, Floor,
		};

		ObjectType object_type= ObjectType::None;
		unsigned int object_index;
		m_Vec3 pos;
	};

	struct SpriteEffect
	{
		m_Vec3 pos;
		unsigned char effect_id;
	};

	typedef std::vector<SpriteEffect> SpriteEffects;

	typedef std::unordered_map< Messages::EntityId, MonsterPtr > MonstersContainer;

private:
	void ActivateProcedure( unsigned int procedure_number, Time current_time );
	void TryActivateProcedure( unsigned int procedure_number, Time current_time, Player& player, MessagesSender& messages_sender );
	void ProcedureProcessDestroy( unsigned int procedure_number, Time current_time );
	void ProcedureProcessShoot( unsigned int procedure_number, Time current_time );

	template<class Func>
	void ProcessElementLinks(
		MapData::IndexElement::Type element_type,
		unsigned int index,
		const Func& func );

	HitResult ProcessShot( const m_Vec3& shot_start_point, const m_Vec3& shot_direction_normalized ) const;

	static void PrepareMonsterStateMessage( const Monster& monster, Messages::MonsterState& message );

private:
	const MapDataConstPtr map_data_;
	const GameResourcesConstPtr game_resources_;

	DynamicWalls dynamic_walls_;

	std::vector<ProcedureState> procedures_;

	StaticModels static_models_;

	Rockets rockets_;
	SpriteEffects sprite_effects_;

	MonstersContainer monsters_;
	Messages::EntityId next_monter_id_= 1u;
};

} // PanzerChasm
