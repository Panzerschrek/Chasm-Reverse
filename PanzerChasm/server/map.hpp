#pragma once
#include <unordered_map>

#include <matrix.hpp>

#include "../map_loader.hpp"
#include "../messages_sender.hpp"
#include "../particles.hpp"
#include "../rand.hpp"
#include "../time.hpp"
#include "collision_index.hpp"
#include "backpack.hpp"
#include "fwd.hpp"
#include "movement_restriction.hpp"

namespace PanzerChasm
{

// Server-side map logic here.
class Map final
{
public:
	typedef std::function<void()> MapEndCallback;

	typedef std::unordered_map< EntityId, PlayerPtr > PlayersContainer;

	Map(
		DifficultyType difficulty,
		GameRules game_rules,
		const MapDataConstPtr& map_data,
		const GameResourcesConstPtr& game_resources,
		Time map_start_time,
		MapEndCallback map_end_callback );

	// Construct from save
	Map(
		DifficultyType difficulty,
		GameRules game_rules,
		const MapDataConstPtr& map_data,
		LoadStream& load_stream,
		const GameResourcesConstPtr& game_resources,
		MapEndCallback map_end_callback );

	~Map();

	DifficultyType GetDifficulty() const;

	void Save( SaveStream& save_stream ) const;

	// Returns monster_id for spawned player
	EntityId SpawnPlayer( const PlayerPtr& player );
	void DespawnPlayer( EntityId player_id );

	void Shoot(
		EntityId owner_id,
		unsigned int rocket_id,
		const m_Vec3& from,
		const m_Vec3& normalized_direction,
		Time current_time );

	void PlantMine( const m_Vec3& pos, Time current_time );
	void SpawnBackpack( BackpackPtr backpack );

	void SpawnMonsterBodyPart(
		unsigned char monster_type_id, unsigned char body_part_id,
		const m_Vec3& pos, float angle );

	void PlayMonsterLinkedSound(
		EntityId monster_id,
		unsigned int sound_id );

	void PlayMonsterSound(
		EntityId monster_id,
		unsigned int monster_sound_id );

	void PlayMapEventSound( const m_Vec3& pos, unsigned int sound_id );

	m_Vec3 CollideWithMap(
		const m_Vec3 in_pos, float height, float radius,
		Time tick_delta,
		bool& out_on_floor, MovementRestriction& out_movement_restriction ) const;

	bool CanSee( const m_Vec3& from, const m_Vec3& to ) const;

	const PlayersContainer& GetPlayers() const;

	void ProcessPlayerPosition( Time current_time, EntityId player_monster_id, MessagesSender& messages_sender );
	void Tick( Time current_time, Time last_tick_delta );

	void SendMessagesForNewlyConnectedPlayer( MessagesSender& messages_sender ) const;
	void SendUpdateMessages( MessagesSender& messages_sender ) const;

	void ClearUpdateEvents();

private:
	struct RotatingLightEffect;

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

		bool locked= false;
		bool first_message_printed= false;

		MovementState movement_state= MovementState::None;
		float movement_stage= 0.0f; // stage of current movement state [0; 1]
		Time last_state_change_time= Time::FromSeconds(0);
	};

	// Structure for accumulationg of walls and models transformations from procedures.
	struct Transformation
	{
		m_Mat3 mat;
		float d_z;

		void Clear(){ mat.Identity(); d_z= 0.0f; }
	};

	struct DynamicWall
	{
		Transformation transformation;

		m_Vec2 vert_pos[2];
		float z;
		unsigned char texture_id;
		bool mortal= false;
	};

	typedef std::vector<DynamicWall> DynamicWalls;

	struct StaticModel
	{
		Transformation transformation;
		float transformation_angle_delta;

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

		bool picked= false; // For keys.
		bool mortal= false;
		std::unique_ptr<RotatingLightEffect> linked_rotating_light;
	};

	typedef std::vector<StaticModel> StaticModels;

	struct Item
	{
		m_Vec3 pos;
		unsigned char item_id;
		bool picked_up;
		bool enabled; // Enabled for current difficulty.
	};

	typedef std::vector<Item> Items;

	struct Rocket
	{
		Rocket(){} // Empty constructor - for deserialization
		Rocket(
			EntityId in_rocket_id,
			EntityId in_owner_id,
			unsigned char in_rocket_type_id,
			const m_Vec3& in_start_point,
			const m_Vec3& in_normalized_direction,
			Time in_start_time );

		bool HasInfiniteSpeed( const GameResources& game_resources ) const;

		// Start parameters
		Time start_time= Time::FromSeconds(0);
		m_Vec3 start_point;
		m_Vec3 normalized_direction;
		EntityId rocket_id;
		EntityId owner_id; // owner - monster
		unsigned char rocket_type_id;

		m_Vec3 previous_position;
		float track_length;

		m_Vec3 speed; // For reflecting rockets.
	};

	typedef std::vector<Rocket> Rockets;

	struct Mine
	{
		Time planting_time= Time::FromSeconds(0);
		m_Vec3 pos;
		EntityId id;
		bool turned_on= false;
	};

	typedef std::vector<Mine> Mines;

	struct SpriteEffect
	{
		m_Vec3 pos;
		unsigned char effect_id;
	};

	typedef std::vector<SpriteEffect> SpriteEffects;

	typedef std::unordered_map< EntityId, MonsterBasePtr > MonstersContainer;

	struct RotatingLightEffect
	{
		Time start_time= Time::FromSeconds(0);
		Time end_time= Time::FromSeconds(0);
		float radius;
		float brightness;
	};

	struct LightSource
	{
		Time birth_time= Time::FromSeconds(0);
		m_Vec2 pos;
		float radius;
		float brightness;
		unsigned short turn_on_time_ms;
	};
	typedef std::unordered_map< EntityId, LightSource > LightSourcesContainer;

	struct HitResult
	{
		enum class ObjectType
		{
			None, StaticWall, DynamicWall, Model, Monster, Floor,
		};

		ObjectType object_type= ObjectType::None;

		// monster_id for monsters, 0 for floors, 1 for ceilings, model number
		// for map models, wall number for walls.
		unsigned int object_index;

		m_Vec3 pos;
	};

	struct DamageFiledCell
	{
		unsigned char damage; // 0 - means no damage
		unsigned char z_bottom, z_top; // 64 units/m
	};

private:
	void ActivateProcedure( unsigned int procedure_number, Time current_time );
	void TryActivateProcedure( unsigned int procedure_number, Time current_time, Player& player, MessagesSender& messages_sender );
	void ProcedureProcessDestroy( unsigned int procedure_number, Time current_time );
	void ProcedureProcessShoot( unsigned int procedure_number, Time current_time );
	void ActivateProcedureSwitches( const MapData::Procedure& procedure, bool inverse_animation, Time current_time );
	void DoProcedureImmediateCommands( const MapData::Procedure& procedure, Time current_time );
	void DoProcedureDeactivationCommands( const MapData::Procedure& procedure );
	void DeactivateProcedureLightSources( const MapData::Procedure& procedure );
	void ReturnProcedure( unsigned int procedure_number, Time current_time );

	void ProcessWind( const MapData::Procedure::ActionCommand& command, bool activate );
	void ProcessDeathZone( const MapData::Procedure::ActionCommand& command, bool activate );
	void DestroyModel( unsigned int model_index );
	void DoExplosionDamage( const m_Vec3& explosion_center, float explosion_radius, int base_damage, Time current_time );

	void MoveMapObjects( Time current_time );

	template<class Func>
	void ProcessElementLinks(
		MapData::IndexElement::Type element_type,
		unsigned int index,
		const Func& func );

	HitResult ProcessShot(
		const m_Vec3& shot_start_point,
		const m_Vec3& shot_direction_normalized,
		float max_distance,
		EntityId skip_monster_id ) const;

	bool FindNearestPlayerPos( const m_Vec3& pos, m_Vec3& out_pos ) const;

	float GetFloorLevel( const m_Vec2& pos, float radius= 0.0f ) const;

	EntityId GetNextMonsterId();
	EntityId GetLightSourceId( unsigned int parent_procedure_number, unsigned int light_source_coomand_number ) const;

	static void PrepareMonsterStateMessage( const MonsterBase& monster, Messages::MonsterState& message );
	static void PrepareRocketStateMessage( const Rocket& rocket, Messages::RocketState& message );
	static void PrepareMineBirthMessage( const Mine& mine, Messages::DynamicItemBirth& message );
	static void PrepareBackpackBirthMessage( const Backpack& backpack, EntityId backpack_id, Messages::DynamicItemBirth& message );
	static void PrepareLightSourceBirthMessage( const LightSource& light_source, EntityId light_source_id, Messages::LightSourceBirth& message );

	void EmitModelDestructionEffects( unsigned int model_number );
	void AddParticleEffect( const m_Vec3& pos, ParticleEffect particle_effect );
	void GenParticleEffectForRocketHit( const m_Vec3& pos, unsigned int rocket_type_id );

private:
	const DifficultyType difficulty_;
	const GameRules game_rules_;
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
	Mines mines_;
	std::unordered_map<EntityId, BackpackPtr> backpacks_;
	EntityId next_rocket_id_= 1u; // Common id for rockets, mines, backpacks, etc.

	SpriteEffects sprite_effects_;

	PlayersContainer players_;
	MonstersContainer monsters_; // + players
	EntityId next_monster_id_= 1u;

	LightSourcesContainer light_sources_;

	std::vector<Messages::MonsterBirth> monsters_birth_messages_;
	std::vector<Messages::MonsterDeath> monsters_death_messages_;
	std::vector<Messages::RocketBirth> rockets_birth_messages_;
	std::vector<Messages::RocketDeath> rockets_death_messages_;
	std::vector<Messages::DynamicItemBirth> dynamic_items_birth_messages_;
	std::vector<Messages::DynamicItemDeath> dynamic_items_death_messages_;
	std::vector<Messages::LightSourceBirth> light_sources_birth_messages_;
	std::vector<Messages::LightSourceDeath> light_sources_death_messages_;
	std::vector<Messages::RotatingLightSourceBirth> rotating_light_sources_birth_messages_;
	std::vector<Messages::RotatingLightSourceDeath> rotating_light_sources_death_messages_;
	std::vector<Messages::ParticleEffectBirth> particles_effects_messages_;
	std::vector<Messages::MonsterPartBirth> monsters_parts_birth_messages_;
	std::vector<Messages::MapEventSound> map_events_sounds_messages_;
	std::vector<Messages::MonsterLinkedSound> monster_linked_sounds_messages_;
	std::vector<Messages::MonsterSound> monsters_sounds_messages_;

	// Put large objects here.

	// TODO - compress this fields
	char wind_field_[ MapData::c_map_size * MapData::c_map_size ][2];
	DamageFiledCell death_field_[ MapData::c_map_size * MapData::c_map_size ];

	const CollisionIndex collision_index_;
};

} // PanzerChasm
