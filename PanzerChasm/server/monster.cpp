#include "../game_constants.hpp"
#include "math_utils.hpp"

#include "collisions.hpp"
#include "map.hpp"

#include "monster.hpp"

namespace PanzerChasm
{

Monster::Monster(
	const MapData::Monster& map_monster,
	const float z,
	const GameResourcesConstPtr& game_resources,
	const LongRandPtr& random_generator,
	const Time spawn_time )
	: MonsterBase(
		game_resources,
		map_monster.monster_id,
		m_Vec3( map_monster.pos, z ),
		map_monster.angle )
	, random_generator_(random_generator)
	, current_animation_start_time_(spawn_time)
{
	PC_ASSERT( game_resources_ != nullptr );
	PC_ASSERT( random_generator_ != nullptr );
	PC_ASSERT( monster_id_ < game_resources_->monsters_models.size() );

	current_animation_= GetAnyAnimation( { AnimationId::Idle0, AnimationId::Idle1, AnimationId::Run } );

	health_= game_resources_->monsters_description[ monster_id_ ].life;
}

Monster::~Monster()
{}

void Monster::Tick( const Map& map, const Time current_time, const Time last_tick_delta )
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

	case State::MoveToTarget:
		if( current_time >= target_change_time_ )
			SelectTarget( current_time );
		MoveToTarget( map, last_tick_delta.ToSeconds() );

		current_animation_frame_= animation_frame_unwrapped % frame_count;
		break;

	case State::PainShock:
		if( animation_frame_unwrapped >= frame_count )
		{
			state_= State::MoveToTarget;
			SelectTarget( current_time );
			current_animation_= GetAnimation( AnimationId::Run );
			current_animation_start_time_= current_time;
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
	if( state_ != State::DeathAnimation && state_ != State::Dead )
	{
		health_-= damage;

		if( health_ > 0 )
		{
			if( state_ != State::PainShock )
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

void Monster::MoveToTarget( const Map& map, const float time_delta_s )
{
	const m_Vec2 vec_to_target= target_position_.xy() - pos_.xy();
	const float vec_to_target_length= vec_to_target.Length();

	// Nothing to do, we are on target
	if( vec_to_target_length == 0.0f )
		return;

	const GameResources::MonsterDescription& monster_description= game_resources_->monsters_description[ monster_id_ ];

	const float distance_delta= time_delta_s * float( monster_description.speed ) / 10.0f;

	if( distance_delta >= vec_to_target_length )
	{
		pos_.x= target_position_.x;
		pos_.y= target_position_.y;
	}
	else
	{
		pos_.x+= std::cos(angle_) * distance_delta;
		pos_.y+= std::sin(angle_) * distance_delta;
	}

	const float target_angle= NormalizeAngle( std::atan2( vec_to_target.y, vec_to_target.x ) );
	float target_angle_delta= target_angle - angle_;
	if( target_angle_delta > +Constants::pi )
		target_angle_delta-= Constants::two_pi;
	if( target_angle_delta < -Constants::pi )
		target_angle_delta+= Constants::two_pi;

	if( target_angle_delta != 0.0f )
	{
		const float abs_target_angle_delta= std::abs(target_angle_delta);
		const float angle_delta= time_delta_s * float(monster_description.rotation_speed);

		if( angle_delta >= abs_target_angle_delta )
			angle_= target_angle;
		else
		{
			const float turn_direction= target_angle_delta > 0.0f ? 1.0f : -1.0f;
			angle_= NormalizeAngle( angle_ + turn_direction * angle_delta );
		}
	}

	const float height= GameConstants::player_height; // TODO - select height
	bool on_floor;
	pos_=
		map.CollideWithMap(
			pos_, height, monster_description.w_radius,
			on_floor );
}

void Monster::SelectTarget( const Time current_time )
{
	const float direction= random_generator_->RandAngle();
	const float distance= random_generator_->RandValue( 2.0f, 5.0f );
	const float target_change_interval_s= random_generator_->RandValue( 0.5f, 2.0f );

	target_position_= pos_ + distance * m_Vec3( std::cos(direction), std::sin(direction), 0.0f );
	target_change_time_= current_time + Time::FromSeconds(target_change_interval_s);
}

} // namespace PanzerChasm
