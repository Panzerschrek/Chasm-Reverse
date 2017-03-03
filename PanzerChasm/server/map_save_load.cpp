#include "map.hpp"

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

} // namespace PanzerChasm
