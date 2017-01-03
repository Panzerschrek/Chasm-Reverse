#include "../game_constants.hpp"
#include "../game_resources.hpp"
#include "../map_loader.hpp"
#include "../math_utils.hpp"
#include "../particles.hpp"

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
		out_wall.texture_id= in_wall.texture_id;
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

const MapState::MonstersBodyParts& MapState::GetMonstersBodyParts() const
{
	return monsters_body_parts_;
}

const MapState::MonstersContainer& MapState::GetMonsters() const
{
	return monsters_;
}

const MapState::RocketsContainer& MapState::GetRockets() const
{
	return rockets_;
}

float MapState::GetSpritesFrame() const
{
	return ( last_tick_time_ - map_start_time_ ).ToSeconds() * GameConstants::sprites_animations_frames_per_second;
}

void MapState::Tick( const Time current_time )
{
	const float time_since_map_start_s= ( current_time - map_start_time_ ).ToSeconds();
	const float tick_delta_s= ( current_time - last_tick_time_ ).ToSeconds();

	last_tick_time_= current_time;

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

		effect.frame= time_delta_s * GameConstants::sprites_animations_frames_per_second;

		const GameResources::SpriteEffectDescription& description= game_resources_->sprites_effects_description[ effect.effect_id ];

		if( description.gravity )
			effect.speed.z+= tick_delta_s * GameConstants::particles_vertical_acceleration;
		effect.pos+= tick_delta_s * effect.speed;

		bool force_kill= false;

		if( effect.pos.z < 0.0f )
		{
			if( description.jump )
			{
				effect.pos.z= 0.0f;

				const float c_speed_scale= 0.5f;
				effect.speed.x*= c_speed_scale;
				effect.speed.y*= c_speed_scale;
				effect.speed.z*= -c_speed_scale;
			}

			if( !description.looped || std::abs( effect.speed.z ) < 0.2f )
				force_kill= true;
		}

		const float sprite_frame_count= float( game_resources_->effects_sprites[ effect.effect_id ].frame_count );

		if( force_kill ||
			( !description.looped && effect.frame >= sprite_frame_count ) )
		{
			if( i < sprite_effects_.size() -1u )
				sprite_effects_[i]= sprite_effects_.back();

			sprite_effects_.pop_back();
		}
		else
		{
			effect.frame= std::fmod( effect.frame, sprite_frame_count );
			i++;
		}
	}

	for( unsigned int p= 0u; p < monsters_body_parts_.size(); )
	{
		MonsterBodyPart& part= monsters_body_parts_[p];
		const float time_delta_s= ( current_time - part.start_time ).ToSeconds();

		if( time_delta_s > 20.0f ) // Kill old part
		{
			if( &part != &monsters_body_parts_.back() )
				part= monsters_body_parts_.back();
			monsters_body_parts_.pop_back();

			continue;
		}

		const unsigned int frame= static_cast<unsigned int>( std::round( time_delta_s * GameConstants::animations_frames_per_second ) );

		PC_ASSERT( part.monster_type < game_resources_->monsters_models.size() );
		unsigned int model_frame_count= game_resources_->monsters_models[ part.monster_type ].submodels[ part.body_part_id ].animations[ part.animation ].frame_count;

		if( model_frame_count > 0u )
			part.animation_frame= std::min( frame, model_frame_count - 1u );
		else
			part.animation_frame= 0u;

		p++;
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
	monster.body_parts_mask= message.body_parts_mask;
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
	wall.texture_id= message.texture_id;
}

void MapState::ProcessMessage( const Messages::ItemState& message )
{
	if( message.item_index >= items_.size() )
		return; // Bad index

	Item& item= items_[ message.item_index ];
	item.pos.z= MessageCoordToCoord( message.z );
	item.picked_up= message.picked;
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

	effect.speed= m_Vec3( 0.0f, 0.0f, 0.0f );

	MessagePositionToPosition( message.xyz, effect.pos );

	effect.start_time= last_tick_time_;
}

void MapState::ProcessMessage( const Messages::ParticleEffectBirth& message )
{
	const ParticleEffect effect= static_cast<ParticleEffect>(message.effect_id);

	switch(effect)
	{
	case ParticleEffect::Glass:
		break;
	case ParticleEffect::Blood:
	{
		const unsigned int c_particle_count= 12u;
		sprite_effects_.resize( sprite_effects_.size() + c_particle_count );
		SpriteEffect* const effects= sprite_effects_.data() + sprite_effects_.size() - c_particle_count;

		m_Vec3 pos;
		MessagePositionToPosition( message.xyz, pos );

		for( unsigned int i= 0u; i < c_particle_count; i++ )
		{
			SpriteEffect& effect= effects[i];

			effect.effect_id= static_cast<unsigned char>(Particels::Blood0) + ( random_generator_.Rand() & 1u );
			effect.frame= 0.0f;

			m_Vec3 rand_dir_in_upper_sphere= random_generator_.RandDirection();
			rand_dir_in_upper_sphere.z= std::abs( rand_dir_in_upper_sphere.z );

			effect.speed= rand_dir_in_upper_sphere * random_generator_.RandValue( 0.8f, 1.2f );
			effect.pos= pos + effect.speed / 16.0f; // Shift just a bit from spawn point

			effect.start_time= last_tick_time_;
		}
	}
		break;
	case ParticleEffect::Bullet:
	{
		sprite_effects_.emplace_back();
		SpriteEffect& flash_effect= sprite_effects_.back();

		flash_effect.effect_id= static_cast<unsigned char>(Particels::Bullet);
		flash_effect.frame= 0.0f;

		flash_effect.speed= m_Vec3( 0.0f, 0.0f, 0.0f );

		m_Vec3 pos;
		MessagePositionToPosition( message.xyz, pos );
		flash_effect.pos= pos;
		flash_effect.start_time= last_tick_time_;

		const unsigned int c_particle_count= 3u;
		sprite_effects_.resize( sprite_effects_.size() + c_particle_count );
		SpriteEffect* const effects= sprite_effects_.data() + sprite_effects_.size() - c_particle_count;

		for( unsigned int i= 0u; i < c_particle_count; i++ )
		{
			SpriteEffect& effect= effects[i];

			effect.effect_id= static_cast<unsigned char>(Particels::LightSmoke);
			effect.frame= 0.0f;

			effect.speed= m_Vec3( 0.0f, 0.0f, 0.25f );
			effect.pos= pos + random_generator_.RandPointInSphere( 0.05f ) + m_Vec3( 0.0f, 0.0f, 0.05f );

			effect.start_time= last_tick_time_;
		}
	}
		break;

	case ParticleEffect::Sparkles:
	{
		m_Vec3 pos;
		MessagePositionToPosition( message.xyz, pos );

		const unsigned int c_sparcle_count= 48u;
		sprite_effects_.resize( sprite_effects_.size() + c_sparcle_count );
		SpriteEffect* const effects= sprite_effects_.data() + sprite_effects_.size() - c_sparcle_count;


		for( unsigned int i= 0u; i < c_sparcle_count; i++ )
		{
			SpriteEffect& effect= effects[i];

			effect.effect_id= static_cast<unsigned char>(Particels::Sparcle);
			effect.frame= 0.0f;

			m_Vec3 dir= random_generator_.RandDirection();
			dir.z*= 2.0f;

			effect.speed= dir * random_generator_.RandValue( 1.0f, 2.0f );
			effect.pos= pos;

			effect.start_time= last_tick_time_;
		}

		const unsigned int c_smoke_particle_count= 5u;
		sprite_effects_.resize( sprite_effects_.size() + c_sparcle_count );
		SpriteEffect* const smoke_effects= sprite_effects_.data() + sprite_effects_.size() - c_smoke_particle_count;
		for( unsigned int i= 0u; i < c_smoke_particle_count; i++ )
		{
			SpriteEffect& effect= smoke_effects[i];
			effect.effect_id= static_cast<unsigned char>(Particels::LightSmoke);
			effect.frame= 0.0f;

			effect.speed= m_Vec3( 0.0f, 0.0f, random_generator_.RandValue( 0.5f, 1.5f ) );
			effect.pos= pos + random_generator_.RandPointInSphere( 0.05f ) + m_Vec3( 0.0f, 0.0f, 0.05f );

			effect.start_time= last_tick_time_;
		}
	}
		break;
	case ParticleEffect::Explosion:
	{
		const unsigned int c_fireball_count= 16u;
		sprite_effects_.resize( sprite_effects_.size() + c_fireball_count );
		SpriteEffect* const effects= sprite_effects_.data() + sprite_effects_.size() - c_fireball_count;

		m_Vec3 pos;
		MessagePositionToPosition( message.xyz, pos );

		for( unsigned int i= 0u; i < c_fireball_count; i++ )
		{
			SpriteEffect& effect= effects[i];

			effect.effect_id= static_cast<unsigned char>(Particels::Explosion);
			effect.frame= 0.0f;

			effect.speed= m_Vec3( 0.0f, 0.0f, random_generator_.RandValue( 0.3f, 0.5f ) );
			effect.pos= pos + random_generator_.RandPointInSphere( 0.3f );

			effect.start_time= last_tick_time_;
		}
	}
		break;

	default:
		if( effect >= ParticleEffect::FirstBlowEffect )
		{
			const unsigned int blow_effect_id=
				static_cast<unsigned int>(effect) - static_cast<unsigned int>(ParticleEffect::FirstBlowEffect);

			if( blow_effect_id < game_resources_->sprites_effects_description.size() )
			{
				const unsigned int c_particle_count= 16u;
				sprite_effects_.resize( sprite_effects_.size() + c_particle_count );
				SpriteEffect* const effects= sprite_effects_.data() + sprite_effects_.size() - c_particle_count;

				m_Vec3 pos;
				MessagePositionToPosition( message.xyz, pos );

				for( unsigned int i= 0u; i < c_particle_count; i++ )
				{
					SpriteEffect& effect= effects[i];

					effect.effect_id= blow_effect_id;
					effect.frame= 0.0f;

					effect.speed= random_generator_.RandDirection() * random_generator_.RandValue( 0.5f, 2.5f );
					effect.pos= pos + random_generator_.RandPointInSphere( 0.01f );

					effect.start_time= last_tick_time_;
				}
			}
		}
		break;
	};
}

void MapState::ProcessMessage( const Messages::MonsterPartBirth& message )
{
	monsters_body_parts_.emplace_back();
	MonsterBodyPart& part= monsters_body_parts_.back();

	part.monster_type= message.monster_type;
	part.body_part_id= message.part_id;

	part.angle= MessageAngleToAngle( message.angle );
	MessagePositionToPosition( message.xyz, part.pos );

	part.animation= 0u;
	part.animation_frame= 0u;

	part.start_time= last_tick_time_;
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
