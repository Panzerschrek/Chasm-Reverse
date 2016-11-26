#include "../game_constants.hpp"

#include "collisions.hpp"

#include "monster.hpp"

namespace PanzerChasm
{

Monster::Monster(
	const MapData::Monster& map_monster,
	const float z,
	const GameResourcesConstPtr& game_resources,
	const Time spawn_time )
	: monster_id_( map_monster.monster_id )
	, game_resources_(game_resources)
	, pos_( map_monster.pos, z )
	, angle_( map_monster.angle )
	, current_animation_start_time_(spawn_time)
{
	PC_ASSERT( game_resources_ != nullptr );
	PC_ASSERT( monster_id_ < game_resources_->monsters_models.size() );

	current_animation_= GetAnyAnimation( { AnimationId::Idle0, AnimationId::Idle1, AnimationId::Run } );

	life_= game_resources_->monsters_description[ monster_id_ ].life;
}

Monster::~Monster()
{}

unsigned char Monster::MonsterId() const
{
	return monster_id_;
}

const m_Vec3& Monster::Position() const
{
	return pos_;
}

float Monster::Angle() const
{
	return angle_;
}

unsigned int Monster::CurrentAnimation() const
{
	return current_animation_;
}

unsigned int Monster::CurrentAnimationFrame() const
{
	return current_animation_frame_;
}

void Monster::Tick( const Time current_time )
{
	const Model& model= game_resources_->monsters_models[ monster_id_ ];

	PC_ASSERT( current_animation_ < model.animations.size() );
	PC_ASSERT( model.animations[ current_animation_ ].frame_count > 0u );

	const float time_delta_s= ( current_time - current_animation_start_time_ ).ToSeconds();
	const float frame= time_delta_s * GameConstants::animations_frames_per_second;

	const unsigned int animation_frame_unwrapped= static_cast<unsigned int>( std::round(frame) );
	const unsigned int frame_count= model.animations[ current_animation_ ].frame_count;

	switch( state_ )
	{
	case State::Idle:
	default:
		current_animation_frame_= animation_frame_unwrapped % frame_count;
		break;

	case State::PainShock:
		if( animation_frame_unwrapped >= frame_count )
		{
			state_= State::Idle;
			current_animation_start_time_= current_time;
			current_animation_= GetAnyAnimation( { AnimationId::Idle0, AnimationId::Idle1 } );
			current_animation_frame_= 0u;
		}
		else
			current_animation_frame_= animation_frame_unwrapped;
		break;

	case State::DeathAnimation:
		if( animation_frame_unwrapped >= frame_count )
			state_= State::Dead;
		else
			current_animation_frame_= animation_frame_unwrapped;
		break;

	case State::Dead:
		current_animation_frame_= frame_count - 1u; // Last frame of death animation
		break;
	};
}

void Monster::Hit( const int damage, const Time current_time )
{
	if( state_ == State::Idle || state_ == State::PainShock )
	{
		life_-= damage;

		if( life_ > 0 )
		{
			if( state_ == State::Idle )
			{
				const int animation= GetAnyAnimation( { AnimationId::Pain0, AnimationId::Pain1 } );
				if( animation >= 0 )
				{
					state_= State::PainShock;
					current_animation_= static_cast<unsigned int>(animation);
					current_animation_start_time_= current_time;
				}
				else
				{}// No pain - no gain
			}
		}
		else
		{
			state_= State::DeathAnimation;
			current_animation_start_time_= current_time;

			const int animation= GetAnyAnimation( { AnimationId::Death0, AnimationId::Death1, AnimationId::Death2, AnimationId::Death3 } );
			PC_ASSERT( animation >= 0 );
			current_animation_= static_cast<unsigned int>(animation);
		}
	}
}

bool Monster::TryShot( const m_Vec3& from, const m_Vec3& direction_normalized, m_Vec3& out_pos ) const
{
	if( state_ == State::Dead )
		return false;

	const Model& model= game_resources_->monsters_models[ monster_id_ ];
	const GameResources::MonsterDescription& description= game_resources_->monsters_description[ monster_id_ ];

	return
		RayIntersectCylinder(
			pos_.xy(), description.w_radius,
			pos_.z + model.z_min, pos_.z + model.z_max,
			from, direction_normalized,
			out_pos );
}

int Monster::GetAnimation( const AnimationId id ) const
{
	PC_ASSERT( monster_id_ < game_resources_->monsters_models.size() );
	const Model& model= game_resources_->monsters_models[ monster_id_ ];

	for( const Model::Animation& animation : model.animations )
	{
		if( animation.id == static_cast<unsigned int>(id) )
			return &animation - model.animations.data();
	}
	return -1;
}

int Monster::GetAnyAnimation( const std::initializer_list<AnimationId>& ids ) const
{
	for( const AnimationId id : ids )
	{
		const int  animation= GetAnimation( id );
		if( animation >= 0 )
			return animation;
	}

	return -1;
}

} // namespace PanzerChasm
