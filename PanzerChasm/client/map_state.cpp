#include "../game_constants.hpp"
#include "../math_utils.hpp"

#include "map_state.hpp"

namespace PanzerChasm
{

MapState::MapState(
	const MapDataConstPtr& map,
	const GameResourcesConstPtr& game_resources,
	const Time map_start_time )
	: map_data_(map)
	, game_resources_(game_resources)
	, map_start_time_(map_start_time)
	, last_tick_time_(map_start_time)
{
	PC_ASSERT( map_data_ != nullptr );

	dynamic_walls_.resize( map_data_->dynamic_walls.size() );

	for( unsigned int w= 0u; w < dynamic_walls_.size(); w++ )
	{
		const MapData::Wall& in_wall= map_data_->dynamic_walls[w];
		DynamicWall& out_wall= dynamic_walls_[w];

		out_wall.vert_pos[0]= in_wall.vert_pos[0];
		out_wall.vert_pos[1]= in_wall.vert_pos[1];
		out_wall.z= 0.0f;
	}

	static_models_.resize( map_data_->static_models.size() );
	for( unsigned int m= 0u; m < static_models_.size(); m++ )
	{
		const MapData::StaticModel& in_model= map_data_->static_models[m];
		StaticModel& out_model= static_models_[m];

		out_model.pos= m_Vec3( in_model.pos, 0.0f );
		out_model.angle= in_model.angle;
		out_model.model_id= in_model.model_id;
		out_model.animation_frame= 0u;
	}

	items_.resize( map_data_->items.size() );
	for( unsigned int i= 0u; i < items_.size(); i++ )
	{
		const MapData::Item& in_item= map_data_->items[i];
		Item& out_item= items_[i];

		out_item.pos= m_Vec3( in_item.pos, 0.0f );
		out_item.angle= in_item.angle;
		out_item.item_id= in_item.item_id;
		out_item.picked_up= false;
		out_item.animation_frame= 0;
	}
}

MapState::~MapState()
{}

const MapState::DynamicWalls& MapState::GetDynamicWalls() const
{
	return dynamic_walls_;
}

const MapState::StaticModels& MapState::GetStaticModels() const
{
	return static_models_;
}

const MapState::Items& MapState::GetItems() const
{
	return items_;
}

const MapState::SpriteEffects& MapState::GetSpriteEffects() const
{
	return sprite_effects_;
}

const MapState::MonstersContainer& MapState::GetMonsters() const
{
	return monsters_;
}

const MapState::RocketsContainer& MapState::GetRockets() const
{
	return rockets_;
}

void MapState::Tick( const Time current_time )
{
	last_tick_time_= current_time;

	const float time_since_map_start_s= ( current_time - map_start_time_ ).ToSeconds();
	for( Item& item : items_ )
	{
		const unsigned int animation_frame=
			static_cast<unsigned int>( std::round( GameConstants::animations_frames_per_second * time_since_map_start_s ) );

		if( item.item_id < game_resources_->items_models.size() )
			item.animation_frame= animation_frame % game_resources_->items_models[ item.item_id ].frame_count;
		else
			item.animation_frame= 0;
	}

	for( unsigned int i= 0u; i < sprite_effects_.size(); )
	{
		SpriteEffect& effect= sprite_effects_[i];

		const float time_delta_s= ( current_time - effect.start_time ).ToSeconds();

		effect.frame= time_delta_s * GameConstants::animations_frames_per_second;

		if( effect.frame >= float( game_resources_->effects_sprites[ effect.effect_id ].frame_count ) )
		{
			if( i < sprite_effects_.size() -1u )
				sprite_effects_[i]= sprite_effects_.back();

			sprite_effects_.pop_back();
		}
		else
			i++;
	}

	for( RocketsContainer::value_type& rocket_value : rockets_ )
	{
		Rocket& rocket= rocket_value.second;

		const float time_delta_s= ( current_time - rocket.start_time ).ToSeconds();
		const float frame= time_delta_s * GameConstants::animations_frames_per_second;

		PC_ASSERT( rocket.rocket_id < game_resources_->rockets_models.size() );

		unsigned int model_frame_count= game_resources_->rockets_models[ rocket.rocket_id ].frame_count;
		if( model_frame_count != 0u )
			rocket.frame= static_cast<unsigned int>( std::round( frame ) ) % model_frame_count;
		else
			rocket.frame= 0u;
	}
}

void MapState::ProcessMessage( const Messages::MonsterState& message )
{
	const auto it= monsters_.find( message.monster_id );
	if( it == monsters_.end() )
		return;

	Monster& monster= it->second;

	MessagePositionToPosition( message.xyz, monster.pos );
	monster.angle= MessageAngleToAngle( message.angle );
	monster.monster_id= message.monster_type;
	monster.animation= message.animation;
	monster.animation_frame= message.animation_frame;
}

void MapState::ProcessMessage( const Messages::WallPosition& message )
{
	if( message.wall_index >= dynamic_walls_.size() )
		return; // Bad wall index.

	DynamicWall& wall= dynamic_walls_[ message.wall_index ];

	MessagePositionToPosition( message.vertices_xy[0], wall.vert_pos[0] );
	MessagePositionToPosition( message.vertices_xy[1], wall.vert_pos[1] );
	wall.z= MessageCoordToCoord( message.z );
}

void MapState::ProcessMessage( const Messages::StaticModelState& message )
{
	if( message.static_model_index >= static_models_.size() )
		return;

	StaticModel& static_model= static_models_[ message.static_model_index ];

	static_model.angle= MessageAngleToAngle( message.angle );
	MessagePositionToPosition( message.xyz, static_model.pos );
	static_model.model_id= message.model_id;

	static_model.animation_frame= message.animation_frame;
}

void MapState::ProcessMessage( const Messages::SpriteEffectBirth& message )
{
	if( message.effect_id >= game_resources_->sprites_effects_description.size() )
		return;

	sprite_effects_.emplace_back();
	SpriteEffect& effect= sprite_effects_.back();

	effect.effect_id= message.effect_id;
	effect.frame= 0.0f;

	MessagePositionToPosition( message.xyz, effect.pos );

	effect.start_time= last_tick_time_;
}

void MapState::ProcessMessage( const Messages::MonsterBirth& message )
{
	auto it= monsters_.find( message.monster_id );
	if( it == monsters_.end() )
		it= monsters_.emplace( message.monster_id, Monster() ).first;

	ProcessMessage( message.initial_state );
}

void MapState::ProcessMessage( const Messages::MonsterDeath& message )
{
	monsters_.erase( message.monster_id );
}

void MapState::ProcessMessage( const Messages::RocketState& message )
{
	const auto it= rockets_.find( message.rocket_id );
	if( it == rockets_.end() )
		return;

	Rocket& rocket= it->second;

	MessagePositionToPosition( message.xyz, rocket.pos );

	for( unsigned int j= 0u; j < 2u; j++ )
		rocket.angle[j]= MessageAngleToAngle( message.angle[j] );
}

void MapState::ProcessMessage( const Messages::RocketBirth& message )
{
	const auto it= rockets_.find( message.rocket_id );
	if( it != rockets_.end() )
		return;

	const auto inserted_it= rockets_.emplace( message.rocket_id, Rocket() ).first;
	inserted_it->second.rocket_id= message.rocket_type;

	inserted_it->second.start_time= last_tick_time_;
	inserted_it->second.frame= 0u;

	ProcessMessage( static_cast<const Messages::RocketState&>( message ) );
}

void MapState::ProcessMessage( const Messages::RocketDeath& message )
{
	rockets_.erase( message.rocket_id );
}

} // namespace PanzerChasm
