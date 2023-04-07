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
		out_model.visible= true;
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

const MapState::Gibs& MapState::GetGibs() const
{
	return gibs_;
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

const MapState::DynamicItemsContainer& MapState::GetDynamicItems() const
{
	return dynamic_items_;
}

const MapState::LightFlashes& MapState::GetLightFlashes() const
{
	return light_flashes_;
}

const MapState::LightSourcesContainer& MapState::GetLightSources() const
{
	return light_sources_;
}

const MapState::DirectedLightSourcesContainer& MapState::GetDirectedLightSources() const
{
	return directed_light_sources_;
}

void MapState::GetFullscreenBlend( m_Vec3& out_color, float& out_alpha ) const
{
	if( fullscreen_blend_effects_.empty() )
	{
		out_color= m_Vec3( 0.0f, 0.0f, 0.0f );
		out_alpha= 0.0f;
		return;
	}

	out_color= m_Vec3( 0.0f, 0.0f, 0.0f );
	float alpha_sum= 0.0f;

	// Calculate color, proportional to alpha.
	for( const FullscreenBlendEffect& effect : fullscreen_blend_effects_ )
	{
		const float effect_age_s= ( last_tick_time_ - effect.birth_time ).ToSeconds();
		const float alpha= std::max( 0.0f, effect.intensity * std::exp( -2.5f * effect_age_s ) );
		alpha_sum+= alpha;
		const float weight= alpha;

		for( unsigned int j= 0u; j < 3; j++ )
			out_color.ToArr()[j]+= float( game_resources_->palette[ effect.color_index * 3u + j ] ) * weight / 255.0f;
	}

	if( alpha_sum > 0.0f )
	{
		out_color/= alpha_sum;
		out_alpha= std::min( 0.8f, alpha_sum );
	}
	else
		out_alpha= 0.0f;
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
		if( item.item_id < game_resources_->items_models.size() )
		{
			const unsigned int frame_count= game_resources_->items_models[ item.item_id ].frame_count;
			if( frame_count > 1u )
			{
				// I don't know why, but in original game first and last frames of looped animations are same.
				// So, just skip last frame.
				const unsigned int animation_frame=
					static_cast<unsigned int>( GameConstants::animations_frames_per_second * time_since_map_start_s );
				item.animation_frame= animation_frame % ( game_resources_->items_models[ item.item_id ].frame_count - 1u );
			}
			else
				item.animation_frame= 0u;
		}
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
			( !description.looped && effect.frame >= sprite_frame_count ) ||
			time_delta_s > 10.0f )
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

	for( unsigned int g= 0u; g < gibs_.size(); )
	{
		Gib& gib= gibs_[g];

		gib.speed.z+= tick_delta_s * GameConstants::particles_vertical_acceleration;
		gib.pos+= tick_delta_s * gib.speed;

		if( gib.pos.z < 0.0f )
		{
			const float c_speed_scale= 0.5f;
			gib.speed.x*= c_speed_scale;
			gib.speed.y*= c_speed_scale;
			gib.speed.z*= -c_speed_scale;
			gib.pos.z= 0.0f;
		}

		const float time_delta_s= ( current_time - gib.start_time ).ToSeconds();

		// Update gib angles, if it is not totaly on floor.
		if( !( gib.pos.z < 0.2f && std::abs( gib.speed.z ) < 0.3f ) )
		{
			gib.angle_x= ( time_delta_s + gib.time_phase ) * 7.0f;
			gib.angle_z= ( time_delta_s + gib.time_phase ) * 5.0f;
		}

		if( time_delta_s > 8.0f )
		{
			if( g < gibs_.size() - 1u )
				gibs_[g]= gibs_.back();
			gibs_.pop_back();
		}
		else
			g++;
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

		// Animate part.

		PC_ASSERT( part.monster_type < game_resources_->monsters_models.size() );
		const auto& animations= game_resources_->monsters_models[ part.monster_type ].submodels[ part.body_part_id ].animations;

		if( animations.size() >= 2u )
		{
			const unsigned int frame_count_0= animations[0].frame_count;
			const unsigned int frame_count_1= animations[1].frame_count;
			const unsigned int frame= static_cast<unsigned int>( std::round( time_delta_s * GameConstants::animations_frames_per_second ) );

			// Play animation 0, then play animation 1, then stop.
			if( frame < frame_count_0 )
			{
				part.animation= 0u;
				part.animation_frame= frame;
			}
			else
			{
				part.animation= 1u;
				if( frame_count_1 > 0u )
					part.animation_frame= std::min( frame - frame_count_0, frame_count_1 - 1u );
				else
					part.animation_frame= 0u;
			}
		}
		else
		{
			part.animation= 0u;
			part.animation_frame= 0u;
		}

		// Move part.
		part.speed.z+= GameConstants::vertical_acceleration * tick_delta_s;
		part.pos+= part.speed * tick_delta_s;

		if( part.pos.z < 0.0f )
		{
			part.pos.z= 0.0f;
			part.speed.z= std::abs( part.speed.z );

			const float c_speed_scale= 0.8f;
			part.speed*= c_speed_scale;
		}

		if( part.speed.xy().SquareLength() < 0.05f * 0.05f )
			part.speed.x= part.speed.y= 0.0f;

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

	for( DynamicItemsContainer::value_type& item_value : dynamic_items_ )
	{
		DynamicItem& item= item_value.second;

		const float time_delta_s= ( current_time - item.birth_time ).ToSeconds();

		const unsigned int animation_frame=
			static_cast<unsigned int>( std::round( GameConstants::animations_frames_per_second * time_delta_s ) );

		if( item.item_type_id < game_resources_->items_models.size() )
			item.frame= animation_frame % game_resources_->items_models[ item.item_type_id ].frame_count;
		else
			item.frame= 0u;

		if( item.item_type_id == GameConstants::mine_item_id )
		{
			// Make mines flashing.
			// TODO - also, change active polygons groups for model.
			if( time_delta_s > GameConstants::mines_preparation_time_s )
				item.fullbright= ( ( animation_frame / 10u ) & 1u ) != 0u;
			else
				item.fullbright= false;
		}
		else
			item.fullbright= false;

		if( item.item_type_id == GameConstants::backpack_item_id )
			item.angle= Constants::pi * time_delta_s;
		else
			item.angle= 0.0f;
	}

	for( unsigned int i= 0u; i < light_flashes_.size(); )
	{
		LightFlash& light_flash= light_flashes_[i];

		const float c_light_life_time_s= 0.4f; // TODO - calibrate this
		const float age_s= ( current_time - light_flash.birth_time ).ToSeconds();

		if( age_s > c_light_life_time_s )
		{
			if( i + 1u != light_flashes_.size() )
				light_flash= light_flashes_.back();
			light_flashes_.pop_back();
			continue;
		}

		if( age_s < c_light_life_time_s * 0.5f )
			light_flash.intensity= age_s * 2.0f / c_light_life_time_s;
		else
			light_flash.intensity= ( c_light_life_time_s - age_s ) * 2.0f / c_light_life_time_s;
		light_flash.intensity= light_flash.intensity * light_flash.intensity; // Make quadratic.

		i++;
	}

	for( DirectedLightSourcesContainer::value_type& light_value : directed_light_sources_ )
	{
		DirectedLightSource& light_source= light_value.second;

		light_source.direction= static_models_[ light_value.first ].angle;
	}

	for( unsigned int i= 0u; i < fullscreen_blend_effects_.size(); )
	{
		FullscreenBlendEffect& effect= fullscreen_blend_effects_[i];

		// Kill old effects.
		if( ( current_time - effect.birth_time ).ToSeconds() > 4.0f )
		{
			if( i + 1u != fullscreen_blend_effects_.size() )
				effect= fullscreen_blend_effects_.back();
			fullscreen_blend_effects_.pop_back();
			continue;
		}
		else
			i++;
	}
}

void MapState::ProcessMessage( const Messages::MonsterState& message )
{
	const auto it= monsters_.find( message.monster_id );
	if( it == monsters_.end() )
		return;

	if( message.monster_type >= game_resources_->monsters_models.size() )
		return;
	const Model& model= game_resources_->monsters_models[ message.monster_type ];

	Monster& monster= it->second;

	MessagePositionToPosition( message.xyz, monster.pos );
	monster.angle= MessageAngleToAngle( message.angle );
	monster.monster_id= message.monster_type;
	monster.body_parts_mask= message.body_parts_mask;
	monster.is_fully_dead= message.is_fully_dead;
	monster.is_invisible= message.is_invisible;
	monster.color= message.color;

	monster.animation= 0u;
	monster.animation_frame= 0u;
	if( message.animation < model.animations.size() )
	{
		monster.animation= message.animation;
		if( message.animation_frame < model.animations[ monster.animation ].frame_count )
			monster.animation_frame= message.animation_frame;
	}
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

	static_model.animation_frame= 0u;
	if( static_model.model_id < map_data_->models.size() )
	{
		if( message.animation_frame < map_data_->models[ static_model.model_id ].frame_count )
			static_model.animation_frame= message.animation_frame;
	}

	static_model.visible= message.visible;
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
			else
			{
				const unsigned int gibs_effect_id=
					static_cast<unsigned int>(effect) - static_cast<unsigned int>(ParticleEffect::FirstBlowEffect);
				if( gibs_effect_id >= GameResources::c_first_gib_number )
				{
					const unsigned int c_gibs_count= 8u;
					gibs_.resize( gibs_.size() + c_gibs_count );
					Gib* const gibs= gibs_.data() + gibs_.size() - c_gibs_count;

					m_Vec3 pos;
					MessagePositionToPosition( message.xyz, pos );

					for( unsigned int i= 0u; i < c_gibs_count; i++ )
					{
						Gib& gib= gibs[i];
						gib.start_time= last_tick_time_;
						gib.speed= random_generator_.RandDirection() * random_generator_.RandValue( 1.0f, 1.5f );
						gib.pos= pos + random_generator_.RandPointInSphere( 0.01f );
						gib.angle_x= gib.angle_z= 0.0f;
						gib.time_phase= random_generator_.RandValue( 16.0f );
						gib.gib_id= gibs_effect_id - GameResources::c_first_gib_number;
					}
				}
			}
		}
		break;
	};

	// Try spawn light flash.
	switch(effect)
	{
	case ParticleEffect::Glass:
	case ParticleEffect::Blood:
	case ParticleEffect::FirstBlowEffect:
		break;

	case ParticleEffect::Bullet:
	case ParticleEffect::Sparkles:
		case ParticleEffect::Explosion:
		{
			m_Vec2 pos;
			MessagePositionToPosition( message.xyz, pos );
			SpawnLightFlash( pos );
		}
		break;
	};
}

void MapState::ProcessMessage( const Messages::FullscreenBlendEffect& message )
{
	fullscreen_blend_effects_.emplace_back();
	FullscreenBlendEffect& effect= fullscreen_blend_effects_.back();
	effect.birth_time= last_tick_time_;
	effect.color_index= message.color_index;
	effect.intensity= static_cast<float>( message.intensity ) / 255.0f;
}

void MapState::ProcessMessage( const Messages::MonsterPartBirth& message )
{
	if( message.monster_type >= game_resources_->monsters_models.size() )
		return;
	const Model& monster_model= game_resources_->monsters_models[ message.monster_type ];
	if( message.part_id >= monster_model.submodels.size() )
		return;
	if( monster_model.submodels[ message.part_id ].frame_count == 0u )
		return;

	monsters_body_parts_.emplace_back();
	MonsterBodyPart& part= monsters_body_parts_.back();

	part.monster_type= message.monster_type;
	part.body_part_id= message.part_id;

	part.angle= MessageAngleToAngle( message.angle );
	MessagePositionToPosition( message.xyz, part.pos );

	part.animation= 0u;
	part.animation_frame= 0u;

	const float rand_speed= random_generator_.RandValue( 0.3f, 0.6f );
	part.speed.x= std::cos( -part.angle ) * rand_speed;
	part.speed.y= std::sin( -part.angle ) * rand_speed;
	part.speed.z= 0.0f;

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

void MapState::ProcessMessage( const Messages::DynamicItemBirth& message )
{
	const auto it= dynamic_items_.find( message.item_id );
	if( it != dynamic_items_.end() )
		return;

	DynamicItem& item= dynamic_items_.emplace( message.item_id, DynamicItem() ).first->second;

	MessagePositionToPosition( message.xyz, item.pos );
	item.birth_time= last_tick_time_;
	item.frame= 0u;
	item.item_type_id= message.item_type_id;
	item.angle= 0.0f;
	item.fullbright= false;
}

void MapState::ProcessMessage( const Messages::DynamicItemUpdate& message )
{
	const auto it= dynamic_items_.find( message.item_id );
	if( it == dynamic_items_.end() )
		return;

	MessagePositionToPosition( message.xyz, it->second.pos );
}

void MapState::ProcessMessage( const Messages::DynamicItemDeath& message )
{
	dynamic_items_.erase( message.item_id );
}

void MapState::ProcessMessage( const Messages::LightSourceBirth& message )
{
	const auto it= light_sources_.find( message.light_source_id );
	if( it != light_sources_.end() )
		return;

	LightSource& source= light_sources_.emplace( message.light_source_id, LightSource() ).first->second;

	MessagePositionToPosition( message.xy, source.pos );
	source.birth_time= last_tick_time_;
	source.intensity= float(message.brightness);
	source.radius= MessageCoordToCoord( message.radius );
}

void MapState::ProcessMessage( const Messages::LightSourceDeath& message )
{
	light_sources_.erase( message.light_source_id );
}

void MapState::SpawnLightFlash( const m_Vec2& pos )
{
	light_flashes_.emplace_back();
	LightFlash& light= light_flashes_.back();

	light.pos= pos;
	light.birth_time= last_tick_time_;
	light.intensity= 0.0f;
	// TODO - use blinking
}

void MapState::ProcessMessage( const Messages::RotatingLightSourceBirth& message )
{
	const auto it= directed_light_sources_.find( message.light_source_id );
	if( it != directed_light_sources_.end() )
		return;

	DirectedLightSource& source=
		directed_light_sources_.emplace( message.light_source_id, DirectedLightSource() ).first->second;

	MessagePositionToPosition( message.xy, source.pos );
	source.intensity= float(message.brightness);
	source.radius= MessageCoordToCoord( message.radius );
	source.direction= 0.0f;
}

void MapState::ProcessMessage( const Messages::RotatingLightSourceDeath& message )
{
	directed_light_sources_.erase( message.light_source_id );
}

} // namespace PanzerChasm
