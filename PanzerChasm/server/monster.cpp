#include "../game_constants.hpp"
#include "../math_utils.hpp"
#include "../sound/sound_id.hpp"

#include "map.hpp"
#include "player.hpp"

#include "monster.hpp"

namespace PanzerChasm
{

static const m_Vec3 g_see_point_delta( 0.0f, 0.0f, 0.5f );

// TODO - use different attack points for differnet mosnters.
// In original game this points are hardcoded.
static const m_Vec3 g_shoot_point_delta( 0.0f, 0.0f, 0.5f );

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

void Monster::Tick(
	Map& map,
	const EntityId monster_id,
	const Time current_time,
	const Time last_tick_delta )
{
	const GameResources::MonsterDescription& description= game_resources_->monsters_description[ monster_id_ ];
	const Model& model= game_resources_->monsters_models[ monster_id_ ];

	PC_ASSERT( current_animation_ < model.animations.size() );
	PC_ASSERT( model.animations[ current_animation_ ].frame_count > 0u );

	const float time_delta_s= ( current_time - current_animation_start_time_ ).ToSeconds();
	const float frame= time_delta_s * GameConstants::animations_frames_per_second;

	const unsigned int animation_frame_unwrapped= static_cast<unsigned int>( std::round(frame) );
	const unsigned int frame_count= model.animations[ current_animation_ ].frame_count;

	// Update target position if target moves.
	const MonsterBasePtr target= target_.monster.lock();
	float distance_for_melee_attack= Constants::max_float;
	if( target != nullptr )
	{
		target_position_= target->Position();

		distance_for_melee_attack=
			( pos_ - target_position_ ).xy().Length() - game_resources_->monsters_description[ target->MonsterId() ].w_radius;
	}

	const float last_tick_delta_s= last_tick_delta.ToSeconds();

	FallDown( last_tick_delta_s );

	switch( state_ )
	{
	case State::Idle:
		if( SelectTarget( map, current_time ) )
		{
			map.PlayMonsterSound( monster_id, Sound::MonsterSoundId::Alarmed );

			state_= State::MoveToTarget;
			current_animation_= GetAnimation( AnimationId::Run );
			current_animation_start_time_= current_time;
			current_animation_frame_= 0u;
		}
		else
			current_animation_frame_= animation_frame_unwrapped % frame_count;
		break;

	case State::MoveToTarget:
	{
		// Try melee attack.
		if( target != nullptr &&
			distance_for_melee_attack <= description.attack_radius )
		{
			const int animation= SelectMeleeAttackAnimation();
			if( animation >= 0 )
			{
				state_= State::MeleeAttack;
				current_animation_= animation;
				current_animation_start_time_= current_time;
				current_animation_frame_= 0u;
				attack_was_done_= false;
			}
		}
		if( state_ == State::MoveToTarget )
		{
			if( current_time >= target_change_time_ )
			{
				if( description.rock >= 0 && target != nullptr &&
					have_right_hand_ && // Monster hold weapon in right hand
					map.CanSee( pos_ + g_see_point_delta, target->Position() + g_see_point_delta ) )
				{
					state_= State::RemoteAttack;
					current_animation_= GetAnimation( AnimationId::RemoteAttack );
					current_animation_start_time_= current_time;
					current_animation_frame_= 0u;
					attack_was_done_= false;
				}
				else
					SelectTarget( map, current_time );
			}

			if( state_ == State::MoveToTarget )
			{
				MoveToTarget( last_tick_delta_s );
				current_animation_frame_= animation_frame_unwrapped % frame_count;
			}
		}
	}
		break;

	case State::PainShock:
		if( animation_frame_unwrapped >= frame_count )
		{
			state_= State::MoveToTarget;
			SelectTarget( map, current_time );
			current_animation_= GetAnimation( AnimationId::Run );
			current_animation_start_time_= current_time;
			current_animation_frame_= 0u;
		}
		else
			current_animation_frame_= animation_frame_unwrapped;
		break;

	case State::MeleeAttack:
		if( animation_frame_unwrapped >= frame_count )
		{
			state_= State::MoveToTarget;
			SelectTarget( map, current_time );
			current_animation_= GetAnimation( AnimationId::Run );
			current_animation_start_time_= current_time;
			current_animation_frame_= 0u;
		}
		else
		{
			RotateToTarget( last_tick_delta_s );

			if( animation_frame_unwrapped >= frame_count / 2u &&
				!attack_was_done_ &&
				target != nullptr &&
				distance_for_melee_attack <= description.attack_radius  )
			{
				target->Hit( description.kick, map, target_.monster_id, current_time );
				map.PlayMonsterSound( monster_id, Sound::MonsterSoundId::MeleeAttack );

				attack_was_done_= true;
			}

			current_animation_frame_= animation_frame_unwrapped;
		}
		break;

	case State::RemoteAttack:
		if( animation_frame_unwrapped >= frame_count )
		{
			state_= State::MoveToTarget;
			SelectTarget( map, current_time );
			current_animation_= GetAnimation( AnimationId::Run );
			current_animation_start_time_= current_time;
			current_animation_frame_= 0u;
		}
		else
		{
			RotateToTarget( last_tick_delta_s );

			if( animation_frame_unwrapped >= frame_count / 2u &&
				!attack_was_done_ &&
				target != nullptr )
			{
				const m_Vec3 shoot_pos= pos_ + g_shoot_point_delta;

				const m_Vec3 actual_dir= target->Position() + g_see_point_delta - shoot_pos;
				m_Vec3 dir( std::cos(angle_), std::sin(angle_), actual_dir.z / actual_dir.xy().Length() );
				dir.Normalize();

				PC_ASSERT( description.rock >= 0 );
				map.Shoot( monster_id, description.rock, shoot_pos, dir, current_time );
				map.PlayMonsterSound( monster_id, Sound::MonsterSoundId::RemoteAttack );

				attack_was_done_= true;
			}

			current_animation_frame_= animation_frame_unwrapped;
		}
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

void Monster::Hit(
	const int damage,
	Map& map,
	const EntityId monster_id,
	const Time current_time)
{
	if( state_ != State::DeathAnimation && state_ != State::Dead )
	{
		health_-= damage;

		if( health_ > 0 )
		{
			// TODO - know, in what states monsters can be in pain and lost body parts.
			if( state_ != State::PainShock &&
				state_ != State::MeleeAttack /*&& state_ != State::RemoteAttack*/ )
			{
				// Pain chance - proportianal do relative damage.
				const unsigned int max_health= game_resources_->monsters_description[ monster_id_ ].life;
				if( random_generator_->RandBool( std::min( damage * 3u, max_health ), max_health ) )
				{
					// Try select pain animation.
					int possible_animations[4];
					unsigned int possible_animation_count= 0u;
					const int pain_animation= GetAnyAnimation( { AnimationId::Pain0, AnimationId::Pain1 } );
					const int  left_hand_lost_animation= GetAnimation( AnimationId:: LeftHandLost );
					const int right_hand_lost_animation= GetAnimation( AnimationId::RightHandLost );
					const int head_lost_animation= GetAnimation( AnimationId::HeadLost );

					if( pain_animation >= 0 )
					{
						possible_animations[ possible_animation_count ]= pain_animation;
						possible_animation_count++;
					}
					if(  have_left_hand_ &&  left_hand_lost_animation >= 0 )
					{
						possible_animations[ possible_animation_count ]=  left_hand_lost_animation;
						possible_animation_count++;
					}
					if( have_right_hand_ && right_hand_lost_animation >= 0 )
					{
						possible_animations[ possible_animation_count ]= right_hand_lost_animation;
						possible_animation_count++;
					}
					if( have_head_ && head_lost_animation >= 0 )
					{
						possible_animations[ possible_animation_count ]= head_lost_animation;
						possible_animation_count++;
					}

					if( possible_animation_count > 0 )
					{
						const int selected_animation= possible_animations[ random_generator_->Rand() % possible_animation_count ];
						if( selected_animation == left_hand_lost_animation  )
						{
							have_left_hand_ = false;
							SpawnBodyPart( map, BodyPartSubmodelId:: LeftHand );
						}
						if( selected_animation == right_hand_lost_animation )
						{
							have_right_hand_= false;
							SpawnBodyPart( map, BodyPartSubmodelId::RightHand );
						}
						if( selected_animation == head_lost_animation )
						{
							have_head_= false;
							SpawnBodyPart( map, BodyPartSubmodelId::Head );
						}

						state_= State::PainShock;
						current_animation_= static_cast<unsigned int>( selected_animation );
						current_animation_start_time_= current_time;
						map.PlayMonsterSound( monster_id, Sound::MonsterSoundId::Pain );
					}
					else
					{}// No pain - no gain
				}
			}
		}
		else
		{
			state_= State::DeathAnimation;
			current_animation_start_time_= current_time;

			const int animation= GetAnyAnimation( { AnimationId::Death2, AnimationId::Death3, AnimationId::Death1, AnimationId::Death0 } );
			PC_ASSERT( animation >= 0 );
			current_animation_= static_cast<unsigned int>(animation);

			map.PlayMonsterSound( monster_id, Sound::MonsterSoundId::Death );
		}
	}
}

void Monster::ClampSpeed( const m_Vec3& clamp_surface_normal )
{
	if( vertical_speed_ > 0.0f &&
		clamp_surface_normal.z < 0.0f )
		vertical_speed_= 0.0f;

	if( vertical_speed_ < 0.0f &&
		clamp_surface_normal.z > 0.0f )
		vertical_speed_= 0.0f;
}

void Monster::SetOnFloor( const bool on_floor )
{
	if( on_floor )
		vertical_speed_= 0.0f;
}

void Monster::Teleport( const m_Vec3& pos, const float angle )
{
	pos_= pos;
	angle_= angle;
	vertical_speed_= 0.0f;
}

void Monster::FallDown( const float time_delta_s )
{
	vertical_speed_+= time_delta_s * GameConstants::vertical_acceleration;
	if( vertical_speed_ > +GameConstants::max_vertical_speed )
		vertical_speed_= +GameConstants::max_vertical_speed;
	if( vertical_speed_ < -GameConstants::max_vertical_speed )
		vertical_speed_= -GameConstants::max_vertical_speed;

	pos_.z+= vertical_speed_ * time_delta_s;
}

void Monster::MoveToTarget( const float time_delta_s )
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

	RotateToTarget( time_delta_s );
}

void Monster::RotateToTarget( float time_delta_s )
{
	const m_Vec2 vec_to_target= target_position_.xy() - pos_.xy();
	if( vec_to_target.SquareLength() == 0.0f )
		return;

	const float target_angle= NormalizeAngle( std::atan2( vec_to_target.y, vec_to_target.x ) );
	float target_angle_delta= target_angle - angle_;
	if( target_angle_delta > +Constants::pi )
		target_angle_delta-= Constants::two_pi;
	if( target_angle_delta < -Constants::pi )
		target_angle_delta+= Constants::two_pi;

	if( target_angle_delta != 0.0f )
	{
		const GameResources::MonsterDescription& monster_description= game_resources_->monsters_description[ monster_id_ ];

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
}

bool Monster::SelectTarget( const Map& map, const Time current_time )
{
	/*
	const float c_half_view_angle= Constants::pi * 0.25f;
	const float c_half_view_angle_cos= std::cos( c_half_view_angle );
	const m_Vec2 view_dir( std::cos(angle_), std::sin(angle_) );
	*/

	float nearest_player_distance= Constants::max_float;
	const Map::PlayersContainer::value_type* nearest_player= nullptr;

	const Map::PlayersContainer& players= map.GetPlayers();
	for( const Map::PlayersContainer::value_type& player_value : players )
	{
		PC_ASSERT( player_value.second != nullptr );
		const Player& player= *player_value.second;

		const m_Vec2 dir_to_player= player.Position().xy() - Position().xy();
		const float distance_to_player= dir_to_player.Length();
		if( distance_to_player == 0.0f )
			continue;

		if( distance_to_player >= nearest_player_distance )
			continue;

		// TODO - add angle check, if needed
		/*
		const float angle_cos= ( dir_to_player * view_dir ) / distance_to_player;
		if( angle_cos < c_half_view_angle_cos )
			continue;
		*/

		if( map.CanSee(
				Position() + g_see_point_delta,
				player.Position() + g_see_point_delta ) )
		{
			nearest_player_distance= distance_to_player;
			nearest_player= &player_value;
		}
	}

	if( nearest_player != nullptr )
	{
		const float target_change_interval_s= 0.8f;

		target_.monster_id= nearest_player->first;
		target_.monster= nearest_player->second;
		target_position_= nearest_player->second->Position();
		target_change_time_= current_time + Time::FromSeconds(target_change_interval_s);

		return true;
	}
	else
	{
		const float direction= random_generator_->RandAngle();
		const float distance= random_generator_->RandValue( 2.0f, 5.0f );
		const float target_change_interval_s= random_generator_->RandValue( 0.5f, 2.0f );

		target_.monster_id= 0u;
		target_.monster= MonsterPtr();

		target_position_= pos_ + distance * m_Vec3( std::cos(direction), std::sin(direction), 0.0f );
		target_change_time_= current_time + Time::FromSeconds(target_change_interval_s);

		return false;
	}
}

int Monster::SelectMeleeAttackAnimation()
{
	int possible_animations[3];
	int possible_animation_count= 0u;

	const int  left_hand_animation= GetAnimation( AnimationId::MeleeAttackLeftHand  );
	const int right_hand_animation= GetAnimation( AnimationId::MeleeAttackRightHand );
	const int head_animation= GetAnimation( AnimationId::MeleeAttackHead );
	if(  have_left_hand_ &&  left_hand_animation >= 0 )
	{
		possible_animations[ possible_animation_count ]=  left_hand_animation;
		possible_animation_count++;
	}
	if( have_right_hand_ && right_hand_animation >= 0 )
	{
		possible_animations[ possible_animation_count ]= right_hand_animation;
		possible_animation_count++;
	}
	if( have_head_ && head_animation >= 0 )
	{
		possible_animations[ possible_animation_count ]= head_animation;
		possible_animation_count++;
	}

	if( possible_animation_count == 0u )
		return -1;

	return possible_animations[ random_generator_->Rand() % possible_animation_count ];
}

void Monster::SpawnBodyPart( Map& map, const unsigned char part_id )
{
	/* Select position for parts spawn.
	 * Each part must be spawned at specific point of monster body.
	 *
	 * This code is experimental. I don`t know, how original game does this.
	 * All constatns are exeprimental.
	 */

	const float c_relative_hands_level= 0.65f;
	const float c_hands_radius= 0.2f;

	const m_Vec2 z_minmax= GetZMinMax();
	m_Vec3 pos= pos_;

	switch( part_id )
	{
	case BodyPartSubmodelId:: LeftHand:
		pos.x+= c_hands_radius * std::cos( angle_ + Constants::half_pi );
		pos.y+= c_hands_radius * std::sin( angle_ + Constants::half_pi );
		pos.z+= z_minmax.x * c_relative_hands_level + z_minmax.y * ( 1.0f - c_relative_hands_level );
		break;

	case BodyPartSubmodelId::RightHand:
		pos.x+= c_hands_radius * std::cos( angle_ - Constants::half_pi );
		pos.y+= c_hands_radius * std::sin( angle_ - Constants::half_pi );
		pos.z+= z_minmax.x * c_relative_hands_level + z_minmax.y * ( 1.0f - c_relative_hands_level );
		break;

	case BodyPartSubmodelId::Head:
		pos.z+= z_minmax.y;
		break;

	default:
		PC_ASSERT(false);
		break;
	};

	map.SpawnMonsterBodyPart( monster_id_, part_id, pos, angle_ );
}

} // namespace PanzerChasm
