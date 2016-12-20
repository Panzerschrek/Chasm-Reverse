#pragma once
#include <unordered_map>

#include "../map_loader.hpp"
#include "../messages_sender.hpp"
#include "../particles.hpp"
#include "../time.hpp"
#include "fwd.hpp"
#include "rand.hpp"

namespace PanzerChasm
{

// Server-side map logic here.
class Map final
{
public:
	typedef std::function<void()> MapEndCallback;

	typedef std::unordered_map< Messages::EntityId, PlayerPtr > PlayersContainer;

	Map(
		const MapDataConstPtr& map_data,
		const GameResourcesConstPtr& game_resources,
		Time map_start_time,
		MapEndCallback map_end_callback );
	~Map();

	// Returns monster_id for spawned player
	Messages::EntityId SpawnPlayer( const PlayerPtr& player );

	void Shoot(
		Messages::EntityId owner_id,
		unsigned int rocket_id,
		const m_Vec3& from,
		const m_Vec3& normalized_direction,
		Time current_time );

	void PlayMonsterLinkedSound(
		Messages::EntityId monster_id,
		unsigned int sound_id );

	void PlayMonsterSound(
		Messages::EntityId monster_id,
		unsigned int monster_sound_id );

	m_Vec3 CollideWithMap(
		const m_Vec3 in_pos, float height, float radius,
		bool& out_on_floor ) const;

	bool CanSee( const m_Vec3& from, const m_Vec3& to ) const;

	const PlayersContainer& GetPlayers() const;

	void ProcessPlayerPosition( Time current_time, Messages::EntityId player_monster_id, MessagesSender& messages_sender );
	void Tick( Time current_time, Time last_tick_delta );

	void SendMessagesForNewlyConnectedPlayer( MessagesSender& messages_sender ) const;
	void SendUpdateMessages( MessagesSender& messages_sender ) const;

	void ClearUpdateEvents();

private:
	struct ProcedureState
	{
		enum class MovementState
		{
			None,
			StartWait,
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
		unsigned char texture_id;
	};

	typedef std::vector<DynamicWall> DynamicWalls;

	struct StaticModel
	{
		m_Vec3 pos;
		float baze_z;
		float angle;

		unsigned char model_id;
		int health;

		enum class AnimationState
		{
			Animation,
			SingleAnimation,
			SingleReverseAnimation,
			SingleFrame,
		};

		AnimationState animation_state;
		Time animation_start_time= Time::FromSeconds(0);
		unsigned int animation_start_frame;

		unsigned int current_animation_frame;
	};

	typedef std::vector<StaticModel> StaticModels;

	struct Item
	{
		m_Vec3 pos;
		unsigned char item_id;
		bool picked_up;
	};

	typedef std::vector<Item> Items;

	struct Rocket
	{
		Rocket(
			Messages::EntityId in_rocket_id,
			Messages::EntityId in_owner_id,
			unsigned char in_rocket_type_id,
			const m_Vec3& in_start_point,
			const m_Vec3& in_normalized_direction,
			Time in_start_time );

		bool HasInfiniteSpeed( const GameResources& game_resources ) const;

		// Start parameters
		Time start_time;
		m_Vec3 start_point;
		m_Vec3 normalized_direction;
		Messages::EntityId rocket_id;
		Messages::EntityId owner_id; // owner - monster
		unsigned char rocket_type_id;

		m_Vec3 previous_position;
		float track_length;
	};

	typedef std::vector<Rocket> Rockets;

	struct SpriteEffect
	{
		m_Vec3 pos;
		unsigned char effect_id;
	};

	typedef std::vector<SpriteEffect> SpriteEffects;

	typedef std::unordered_map< Messages::EntityId, MonsterBasePtr > MonstersContainer;

	struct HitResult
	{
		enum class ObjectType
		{
			None, StaticWall, DynamicWall, Model, Monster, Floor,
		};

		ObjectType object_type= ObjectType::None;
		unsigned int object_index; // monster_id for monster
		m_Vec3 pos;
	};

private:
	void ActivateProcedure( unsigned int procedure_number, Time current_time );
	void TryActivateProcedure( unsigned int procedure_number, Time current_time, Player& player, MessagesSender& messages_sender );
	void ProcedureProcessDestroy( unsigned int procedure_number, Time current_time );
	void ProcedureProcessShoot( unsigned int procedure_number, Time current_time );
	void ActivateProcedureSwitches( const MapData::Procedure& procedure, bool inverse_animation, Time current_time );
	void DoProcedureImmediateCommands( const MapData::Procedure& procedure );
	void DoProcedureDeactivationCommands( const MapData::Procedure& procedure );

	void ProcessWind( const MapData::Procedure::ActionCommand& command, bool activate );
	void DestroyModel( unsigned int model_index );

	void MoveMapObjects( bool active );

	template<class Func>
	void ProcessElementLinks(
		MapData::IndexElement::Type element_type,
		unsigned int index,
		const Func& func );

	HitResult ProcessShot(
		const m_Vec3& shot_start_point,
		const m_Vec3& shot_direction_normalized,
		Messages::EntityId skip_monster_id ) const;

	float GetFloorLevel( const m_Vec2& pos, float radius= 0.0f ) const;

	Messages::EntityId GetNextMonsterId();

	static void PrepareMonsterStateMessage( const MonsterBase& monster, Messages::MonsterState& message );

	void EmitModelDestructionEffects( unsigned int model_number );
	void AddParticleEffect( const m_Vec3& pos, ParticleEffect particle_effect );
	void GenParticleEffectForRocketHit( const m_Vec3& pos, unsigned int rocket_type_id );

private:
	const MapDataConstPtr map_data_;
	const GameResourcesConstPtr game_resources_;
	const MapEndCallback map_end_callback_;

	const LongRandPtr random_generator_;

	DynamicWalls dynamic_walls_;

	std::vector<ProcedureState> procedures_;

	bool map_end_triggered_= false;

	StaticModels static_models_;
	Items items_;

	Rockets rockets_;
	Messages::EntityId next_rocket_id_= 1u;

	SpriteEffects sprite_effects_;

	PlayersContainer players_;
	MonstersContainer monsters_; // + players
	Messages::EntityId next_monster_id_= 1u;

	std::vector<Messages::RocketBirth> rockets_birth_messages_;
	std::vector<Messages::RocketDeath> rockets_death_messages_;
	std::vector<Messages::ParticleEffectBirth> particles_effects_messages_;
	std::vector<Messages::MapEventSound> map_events_sounds_messages_;
	std::vector<Messages::MonsterLinkedSound> monster_linked_sounds_messages_;
	std::vector<Messages::MonsterSound> monsters_sounds_messages_;

	char wind_field_[ MapData::c_map_size * MapData::c_map_size ][2];
};

} // PanzerChasm
