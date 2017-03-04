#include "map.hpp"
#include "monster.hpp"
#include "player.hpp"

namespace PanzerChasm
{

void Map::Save( SaveLoadBuffer& save_buffer, const Time current_server_time ) const
{
	SaveStream save_stream( save_buffer, current_server_time );

	// Dynamic walls
	save_stream.WriteUInt32( static_cast<uint32_t>( dynamic_walls_.size() ) );
	for( const DynamicWall& wall : dynamic_walls_ )
	{
		save_stream.WriteFloat( wall.vert_pos[0].x );
		save_stream.WriteFloat( wall.vert_pos[0].y );
		save_stream.WriteFloat( wall.vert_pos[1].x );
		save_stream.WriteFloat( wall.vert_pos[1].y );
		save_stream.WriteFloat( wall.z );
		save_stream.WriteUInt8( wall.texture_id );
		save_stream.WriteBool( wall.mortal );
	}

	// Procedures
	save_stream.WriteUInt32( static_cast<uint32_t>( procedures_.size() ) );
	for( const ProcedureState& procedure_state : procedures_ )
	{
		save_stream.WriteBool( procedure_state.locked );
		save_stream.WriteBool( procedure_state.first_message_printed );
		save_stream.WriteUInt32( static_cast<uint32_t>( procedure_state.movement_state ) );
		// save_stream.WriteUInt32( procedure_state.movement_loop_iteration ); // TODO - does this needs ?
		save_stream.WriteFloat( procedure_state.movement_stage );
		save_stream.WriteTime( procedure_state.last_state_change_time );
	}

	// Map end flag
	save_stream.WriteBool( map_end_triggered_ );

	// Static models
	save_stream.WriteUInt32( static_cast<uint32_t>( static_models_.size() ) );
	for( const StaticModel& model : static_models_ )
	{
		// TODO - add save-load methods for vectors
		save_stream.WriteFloat( model.pos.x );
		save_stream.WriteFloat( model.pos.y );
		save_stream.WriteFloat( model.pos.y );
		save_stream.WriteFloat( model.baze_z );
		save_stream.WriteFloat( model.angle );
		save_stream.WriteUInt8( model.model_id );
		save_stream.WriteInt32( model.health );
		save_stream.WriteUInt32( static_cast<uint32_t>(model.animation_state) );
		save_stream.WriteTime( model.animation_start_time );
		save_stream.WriteUInt32( model.animation_start_frame );
		save_stream.WriteUInt32( model.current_animation_frame );
		save_stream.WriteBool( model.picked );
		save_stream.WriteBool( model.mortal );

		// Save optional rotating light
		save_stream.WriteBool( model.linked_rotating_light != nullptr );
		if( model.linked_rotating_light != nullptr )
		{
			save_stream.WriteTime( model.linked_rotating_light->start_time );
			save_stream.WriteTime( model.linked_rotating_light->end_time );
			save_stream.WriteFloat( model.linked_rotating_light->radius );
			save_stream.WriteFloat( model.linked_rotating_light->brightness );
		}
	}

	// Items
	save_stream.WriteUInt32( static_cast<uint32_t>( items_.size() ) );
	for( const Item& item : items_ )
	{
		// TODO - position really needs?
		save_stream.WriteFloat( item.pos.x );
		save_stream.WriteFloat( item.pos.y );
		save_stream.WriteFloat( item.pos.z );
		// TODO - item id is constant. remove it ?
		save_stream.WriteUInt8( item.item_id );
		save_stream.WriteBool( item.picked_up );
		save_stream.WriteBool( item.enabled );
	}

	// Rockets
	save_stream.WriteUInt32( static_cast<uint32_t>( rockets_.size() ) );
	for( const Rocket& rocket : rockets_ )
	{
		save_stream.WriteTime( rocket.start_time );
		save_stream.WriteFloat( rocket.start_point.x );
		save_stream.WriteFloat( rocket.start_point.y );
		save_stream.WriteFloat( rocket.start_point.z );
		save_stream.WriteFloat( rocket.normalized_direction.x );
		save_stream.WriteFloat( rocket.normalized_direction.y );
		save_stream.WriteFloat( rocket.normalized_direction.z );
		save_stream.WriteUInt16( rocket.rocket_id );
		save_stream.WriteUInt16( rocket.owner_id );
		save_stream.WriteUInt8( rocket.rocket_type_id );
		save_stream.WriteFloat( rocket.previous_position.x );
		save_stream.WriteFloat( rocket.previous_position.y );
		save_stream.WriteFloat( rocket.previous_position.z );
		save_stream.WriteFloat( rocket.track_length );
		save_stream.WriteFloat( rocket.speed.x );
		save_stream.WriteFloat( rocket.speed.y );
		save_stream.WriteFloat( rocket.speed.z );
	}

	// Mines
	save_stream.WriteUInt32( static_cast<uint32_t>( mines_.size() ) );
	for( const Mine& mine : mines_ )
	{
		save_stream.WriteTime( mine.planting_time );
		save_stream.WriteFloat( mine.pos.x );
		save_stream.WriteFloat( mine.pos.y );
		save_stream.WriteFloat( mine.pos.z );
		save_stream.WriteUInt16( mine.id );
		save_stream.WriteBool( mine.turned_on );
	}

	// Backpacks
	save_stream.WriteUInt32( static_cast<uint32_t>( backpacks_.size() ) );
	for( const std::unordered_map<EntityId, BackpackPtr>::value_type& backpack_value : backpacks_ )
	{
		const Backpack& backpack = *backpack_value.second;

		save_stream.WriteUInt16( backpack_value.first );

		save_stream.WriteFloat( backpack.pos.x );
		save_stream.WriteFloat( backpack.pos.y );
		save_stream.WriteFloat( backpack.pos.z );
		save_stream.WriteFloat( backpack.vertical_speed );
		save_stream.WriteFloat( backpack.min_z );
		for( unsigned int i= 0u; i < GameConstants::weapon_count; i++ )
		{
			save_stream.WriteBool( backpack.weapon[i] );
			save_stream.WriteUInt8( backpack.ammo[i] );
		}
		save_stream.WriteBool( backpack.red_key   );
		save_stream.WriteBool( backpack.green_key );
		save_stream.WriteBool( backpack.blue_key  );
		save_stream.WriteUInt8( backpack.armor );
	}

	// Next rocket id
	save_stream.WriteUInt16( next_rocket_id_ );

	// Players - do not save it. Save it as monsters

	// Monsters
	save_stream.WriteUInt32( static_cast<uint32_t>( monsters_.size() ) );
	for( const MonstersContainer::value_type& monster_value : monsters_ )
	{
		// TODO
	}

	// Next monster id
	save_stream.WriteUInt16( next_monster_id_ );

	// Light sources
	save_stream.WriteUInt32( static_cast<uint32_t>( light_sources_.size() ) );
	for( const LightSourcesContainer::value_type& light_source_value : light_sources_ )
	{
		const LightSource& light_source= light_source_value.second;

		save_stream.WriteUInt16( light_source_value.first );

		save_stream.WriteTime( light_source.birth_time );
		save_stream.WriteFloat( light_source.pos.x );
		save_stream.WriteFloat( light_source.pos.y );
		save_stream.WriteFloat( light_source.radius );
		save_stream.WriteFloat( light_source.brightness );
		save_stream.WriteUInt16( light_source.turn_on_time_ms );
	}

	// Wind field
	// TODO - optimize large arrays saving
	for( const char* const wind_field_cell : wind_field_ )
	{
		save_stream.WriteInt8( int8_t( wind_field_cell[0] ) );
		save_stream.WriteInt8( int8_t( wind_field_cell[1] ) );
	}

	// Death field
	for( const DamageFiledCell& damage_field_cell : death_field_ )
	{
		save_stream.WriteUInt8( damage_field_cell.damage );
		save_stream.WriteUInt8( damage_field_cell.z_bottom );
		save_stream.WriteUInt8( damage_field_cell.z_top );
	}
}

Map::Map(
	const DifficultyType difficulty,
	const MapDataConstPtr& map_data,
	const SaveLoadBuffer& save_buffer, unsigned int& save_buffer_pos,
	const GameResourcesConstPtr& game_resources,
	const Time map_start_time,
	MapEndCallback map_end_callback )
	: difficulty_(difficulty)
	, map_data_(map_data)
	, game_resources_(game_resources)
	, map_end_callback_( std::move( map_end_callback ) )
	, random_generator_( std::make_shared<LongRand>() )
	, collision_index_( map_data )
{
	PC_ASSERT( map_data_ != nullptr );
	PC_ASSERT( game_resources_ != nullptr );

	LoadStream load_stream( save_buffer, save_buffer_pos, map_start_time );

	// Dynamic walls
	unsigned int dynamic_walls_count;
	load_stream.ReadUInt32( dynamic_walls_count );
	PC_ASSERT( dynamic_walls_count == map_data_->dynamic_walls.size() );
	dynamic_walls_.resize( dynamic_walls_count );
	for( DynamicWall& wall : dynamic_walls_ )
	{
		load_stream.ReadFloat( wall.vert_pos[0].x );
		load_stream.ReadFloat( wall.vert_pos[0].y );
		load_stream.ReadFloat( wall.vert_pos[1].x );
		load_stream.ReadFloat( wall.vert_pos[1].y );
		load_stream.ReadFloat( wall.z );
		load_stream.ReadUInt8( wall.texture_id );
		load_stream.ReadBool( wall.mortal );
	}

	// Procedures
	unsigned int procedures_count;
	load_stream.ReadUInt32( procedures_count );
	PC_ASSERT( procedures_count == map_data_->procedures.size() );
	procedures_.resize( procedures_count );
	for( ProcedureState& procedure_state : procedures_ )
	{
		load_stream.ReadBool( procedure_state.locked );
		load_stream.ReadBool( procedure_state.first_message_printed );
		unsigned int movement_state;
		load_stream.ReadUInt32( movement_state );
		procedure_state.movement_state= static_cast<ProcedureState::MovementState>( movement_state );
		// save_stream.WriteUInt32( procedure_state.movement_loop_iteration ); // TODO - does this needs ?
		load_stream.ReadFloat( procedure_state.movement_stage );
		load_stream.ReadTime( procedure_state.last_state_change_time );
	}

	// Map end flag
	load_stream.ReadBool( map_end_triggered_ );

	// Static models
	unsigned int static_models_count;
	load_stream.ReadUInt32( static_models_count );
	PC_ASSERT( static_models_count == map_data_->static_models.size() );
	static_models_.resize( static_models_count );
	for( StaticModel& model : static_models_ )
	{
		// TODO - add save-load methods for vectors
		load_stream.ReadFloat( model.pos.x );
		load_stream.ReadFloat( model.pos.y );
		load_stream.ReadFloat( model.pos.y );
		load_stream.ReadFloat( model.baze_z );
		load_stream.ReadFloat( model.angle );
		load_stream.ReadUInt8( model.model_id );
		load_stream.ReadInt32( model.health );
		unsigned int animation_state;
		load_stream.ReadUInt32( animation_state );
		model.animation_state= static_cast<StaticModel::AnimationState>(animation_state);
		load_stream.ReadTime( model.animation_start_time );
		load_stream.ReadUInt32( model.animation_start_frame );
		load_stream.ReadUInt32( model.current_animation_frame );
		load_stream.ReadBool( model.picked );
		load_stream.ReadBool( model.mortal );

		// Load optional rotating light
		bool is_rotating_light;
		load_stream.ReadBool( is_rotating_light );
		if( is_rotating_light )
		{
			model.linked_rotating_light.reset( new RotatingLightEffect );
			load_stream.ReadTime( model.linked_rotating_light->start_time );
			load_stream.ReadTime( model.linked_rotating_light->end_time );
			load_stream.ReadFloat( model.linked_rotating_light->radius );
			load_stream.ReadFloat( model.linked_rotating_light->brightness );
		}
	}

	// Items
	unsigned int item_count;
	load_stream.ReadUInt32( item_count );
	PC_ASSERT( item_count == map_data_->items.size() );
	items_.resize( item_count );
	for( Item& item : items_ )
	{
		// TODO - position really needs?
		load_stream.ReadFloat( item.pos.x );
		load_stream.ReadFloat( item.pos.y );
		load_stream.ReadFloat( item.pos.z );
		// TODO - item id is constant. remove it ?
		load_stream.ReadUInt8( item.item_id );
		load_stream.ReadBool( item.picked_up );
		load_stream.ReadBool( item.enabled );
	}

	// Rockets
	unsigned int rocket_count;
	load_stream.ReadUInt32( rocket_count );
	rockets_.resize( rocket_count );
	for( Rocket& rocket : rockets_ )
	{
		load_stream.ReadTime( rocket.start_time );
		load_stream.ReadFloat( rocket.start_point.x );
		load_stream.ReadFloat( rocket.start_point.y );
		load_stream.ReadFloat( rocket.start_point.z );
		load_stream.ReadFloat( rocket.normalized_direction.x );
		load_stream.ReadFloat( rocket.normalized_direction.y );
		load_stream.ReadFloat( rocket.normalized_direction.z );
		load_stream.ReadUInt16( rocket.rocket_id );
		load_stream.ReadUInt16( rocket.owner_id );
		load_stream.ReadUInt8( rocket.rocket_type_id );
		load_stream.ReadFloat( rocket.previous_position.x );
		load_stream.ReadFloat( rocket.previous_position.y );
		load_stream.ReadFloat( rocket.previous_position.z );
		load_stream.ReadFloat( rocket.track_length );
		load_stream.ReadFloat( rocket.speed.x );
		load_stream.ReadFloat( rocket.speed.y );
		load_stream.ReadFloat( rocket.speed.z );
	}

	// Mines
	unsigned int mine_count;
	load_stream.ReadUInt32( mine_count );
	mines_.resize( mine_count );
	for( Mine& mine : mines_ )
	{
		load_stream.ReadTime( mine.planting_time );
		load_stream.ReadFloat( mine.pos.x );
		load_stream.ReadFloat( mine.pos.y );
		load_stream.ReadFloat( mine.pos.z );
		load_stream.ReadUInt16( mine.id );
		load_stream.ReadBool( mine.turned_on );
	}

	// Backpacks
	unsigned int backpack_count;
	load_stream.ReadUInt32( backpack_count );
	for( unsigned int i= 0u; i < backpack_count; i++ )
	{
		EntityId id;
		load_stream.ReadUInt16( id );
		Backpack& backpack= *(backpacks_[id]);

		load_stream.ReadFloat( backpack.pos.x );
		load_stream.ReadFloat( backpack.pos.y );
		load_stream.ReadFloat( backpack.pos.z );
		load_stream.ReadFloat( backpack.vertical_speed );
		load_stream.ReadFloat( backpack.min_z );
		for( unsigned int i= 0u; i < GameConstants::weapon_count; i++ )
		{
			load_stream.ReadBool( backpack.weapon[i] );
			load_stream.ReadUInt8( backpack.ammo[i] );
		}
		load_stream.ReadBool( backpack.red_key   );
		load_stream.ReadBool( backpack.green_key );
		load_stream.ReadBool( backpack.blue_key  );
		load_stream.ReadUInt8( backpack.armor );
	}

	// Next rocket id
	load_stream.ReadUInt16( next_rocket_id_ );

	// Players - do not save it. Save it as monsters

	// Monsters
	unsigned int monster_count;
	load_stream.ReadUInt32( monster_count );
	for( unsigned int i= 0u; i < monster_count; i++ )
	{
		// TODO
	}

	// Next monster id
	load_stream.ReadUInt16( next_monster_id_ );

	// Light sources
	unsigned int light_source_count;
	load_stream.ReadUInt32( light_source_count );
	for( unsigned int i= 0u; i < light_source_count; i++ )
	{
		EntityId id;
		load_stream.ReadUInt16( id );
		LightSource& light_source= light_sources_[id];

		load_stream.ReadTime( light_source.birth_time );
		load_stream.ReadFloat( light_source.pos.x );
		load_stream.ReadFloat( light_source.pos.y );
		load_stream.ReadFloat( light_source.radius );
		load_stream.ReadFloat( light_source.brightness );
		load_stream.ReadUInt16( light_source.turn_on_time_ms );
	}

	// Wind field
	// TODO - optimize large arrays saving
	for( char* const wind_field_cell : wind_field_ )
	{
		load_stream.ReadInt8( reinterpret_cast<int8_t&>(wind_field_cell[0]) );
		load_stream.ReadInt8( reinterpret_cast<int8_t&>(wind_field_cell[1]) );
	}

	// Death field
	for( DamageFiledCell& damage_field_cell : death_field_ )
	{
		load_stream.ReadUInt8( damage_field_cell.damage );
		load_stream.ReadUInt8( damage_field_cell.z_bottom );
		load_stream.ReadUInt8( damage_field_cell.z_top );
	}


	save_buffer_pos= load_stream.GetBufferPos();
}

void MonsterBase::Save( SaveStream& save_stream )
{
	save_stream.WriteBool( have_left_hand_ );
	save_stream.WriteBool( have_right_hand_ );
	save_stream.WriteBool( have_head_ );
	save_stream.WriteBool( fragmented_ );
	save_stream.WriteFloat( pos_.x );
	save_stream.WriteFloat( pos_.y );
	save_stream.WriteFloat( pos_.z );
	save_stream.WriteFloat( angle_ );
	save_stream.WriteInt32( health_ );
	save_stream.WriteUInt32( current_animation_ );
	save_stream.WriteUInt32( current_animation_frame_ );
}

void Monster::Save( SaveStream& save_stream )
{
	MonsterBase::Save( save_stream );

	save_stream.WriteUInt32( static_cast<uint32_t>(state_) );
	save_stream.WriteTime( current_animation_start_time_ );
	save_stream.WriteFloat( vertical_speed_ );
	save_stream.WriteBool( attack_was_done_ );
	save_stream.WriteUInt16( target_.monster_id );
	// monster weak ptr - TODO
	save_stream.WriteFloat( target_position_.x );
	save_stream.WriteFloat( target_position_.y );
	save_stream.WriteFloat( target_position_.z );
	save_stream.WriteTime( target_change_time_ );
}

void Player::Save( SaveStream& save_stream )
{
	MonsterBase::Save( save_stream );

	save_stream.WriteTime( spawn_time_ );
	save_stream.WriteFloat( speed_.x );
	save_stream.WriteFloat( speed_.y );
	save_stream.WriteFloat( speed_.z );
	save_stream.WriteBool( on_floor_ );
	save_stream.WriteBool( noclip_ );
	save_stream.WriteBool( god_mode_ );
	save_stream.WriteBool( teleported_ );
	for( unsigned int i= 0u; i < GameConstants::weapon_count; i++ )
	{
		save_stream.WriteUInt8( ammo_[i] );
		save_stream.WriteBool( have_weapon_[i] );
	}
	save_stream.WriteInt32( armor_ );
	save_stream.WriteBool( have_red_key_   );
	save_stream.WriteBool( have_green_key_ );
	save_stream.WriteBool( have_blue_key_  );
	save_stream.WriteFloat( mevement_acceleration_ );
	save_stream.WriteFloat( movement_direction_ );
	save_stream.WriteBool( jump_pessed_ );
	save_stream.WriteUInt32( static_cast<uint32_t>( state_ ) );
	save_stream.WriteTime( last_state_change_time_ );
	save_stream.WriteUInt32( static_cast<uint32_t>( weapon_state_ ) );
	save_stream.WriteFloat( view_angle_x_ );
	save_stream.WriteFloat( view_angle_z_ );
	save_stream.WriteBool( shoot_pressed_ );
	save_stream.WriteTime( last_shoot_time_ );
	save_stream.WriteUInt32( current_weapon_index_ );
	save_stream.WriteUInt32( requested_weapon_index_ );
	save_stream.WriteFloat( weapon_switch_stage_ );
	save_stream.WriteUInt32( current_weapon_animation_ );
	save_stream.WriteUInt32( current_weapon_animation_frame_ );
	save_stream.WriteTime( weapon_animation_state_change_time_ );
	save_stream.WriteTime( last_pain_sound_time_ );
	save_stream.WriteTime( last_step_sound_time_ );
}

} // namespace PanzerChasm
