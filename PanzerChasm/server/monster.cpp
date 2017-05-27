#include "../game_constants.hpp"
#include "../math_utils.hpp"
#include "../sound/sound_id.hpp"

#include "map.hpp"
#include "player.hpp"

#include "monster.hpp"

namespace PanzerChasm
{

struct ShootInfo
{
	unsigned int count;
	m_Vec3 positions[4];
};

// Shoot points of mosters.
static const ShootInfo g_shoot_info[]=
{
	{ // Player
		1u, { { 0.0f, 0.0f, 0.5f } } // Player is player
	},
	{ // Joker
		1u, { { 0.1f, -0.15f, 0.5f } } // check
	},
	{ // Spider
		1u, { { 0.0f, 0.0f, 0.5f } } // melee only
	},
	{ // Wman
		2u, { { 0.1f, -0.2f, 1.5f }, { 0.1f, 0.2f, 1.5f } } // check
	},
	{ // Scelet
		1u, { { 0.05f, -0.2f, 0.75f } } // check
	},
	{ // Punisher
		1u, { { 0.1f, -0.31f, 0.72f } } // check
	},
	{ // Mong
		1u, { { 0.0f, 0.0f, 0.5f } } // check
	},
	{ // Faust
		1u, { { 0.0f, -0.145f, 0.84f } } // check
	},
	{ // Turret
		1u, { { 0.1f, 0.0f, 0.2f } } // check
	},
	{ // SFag
		1u, { { -0.1f, -0.45f, 1.1f } } // check
	},
	{ // Deadman
		1u, { { 0.0f, 0.0f, 0.5f } }
	},
	{ // Hog
		1u, { { 0.0f, 0.0f, 0.5f } } // melee only
	},
	{ // Alien
		1u, { { 0.0f, -0.25f, 0.65f } } // check
	},
	{ // Mincer
		// TODO - this monster shoot different from other monsters. Write special code for him.
		2u, { { 0.0f, -0.3f, 0.5f }, { 0.0f, 0.3f, 0.5f } } // check
	},
	{ // Jumper
		1u, { { 0.0f, 0.0f, 0.5f } } // melee only
	},
	{ // Viking
		1u, { { 0.0f, 0.0f, 0.5f } } // melee only
	},
	{ // Lion
		1u, { { 0.0f, 0.0f, 0.5f } } // melee only
	},
	{ // Gross
		1u, { { 0.0f, 0.4f, 0.6f } } // check
	},
	{ // Sphinx
		1u, { { 0.6f, 0.0f, 0.5f } } // check
	},
	{ // Phant
		3u, { { 1.0f, -0.25f, 0.7f }, { 1.0f, 0.0f, 0.7f }, { 1.0f, +0.25f, 0.7f } } // check
	},
	{ // Worm
		1u, { { 0.0f, 0.0f, 0.5f } } // melee only
	},
	{
		1u, { { 0.0f, 0.0f, 0.5f } }
	},
	{
		1u, { { 0.0f, 0.0f, 0.5f } }
	},
	{
		1u, { { 0.0f, 0.0f, 0.5f } }
	},
};

static const m_Vec3 g_see_point_delta( 0.0f, 0.0f, 0.5f );

// Position of aiming on target.
static const m_Vec3 g_target_shoot_point_delta( 0.0f, 0.0f, 0.5f );

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
	PC_ASSERT( random_generator_ != nullptr );

	current_animation_= GetIdleAnimation();

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
	const Time animation_duration= Time::FromSeconds( float(frame_count) / GameConstants::animations_frames_per_second );

	// Update target position if target moves.
	const MonsterBasePtr target= target_.monster.lock();
	float distance_for_melee_attack= Constants::max_float;
	bool target_is_alive= false;
	if( target != nullptr )
	{
		target_is_alive= target->Health() > 0;

		if( CanSee( map, target->Position() ) )
		{
			target_.position= target->Position();
			target_.have_position= true;

			distance_for_melee_attack=
				( pos_ - target_.position ).xy().Length() - game_resources_->monsters_description[ target->MonsterId() ].w_radius;
		}
	}

	const float last_tick_delta_s= last_tick_delta.ToSeconds();

	FallDown( last_tick_delta_s );

	switch( state_ )
	{
	case State::Idle:
		if( SelectTarget( map ) )
		{
			map.PlayMonsterSound( monster_id, Sound::MonsterSoundId::Alarmed );
			state_= State::MoveToTarget;
			current_animation_= GetAnimation( AnimationId::Run );
			current_animation_start_time_ = current_time;
		}
		else if( animation_frame_unwrapped >= frame_count )
		{
			const int idle_animation = GetIdleAnimation();
			// Try play boring animation, sometimes. Doubled borng animations is disabled.
			if( random_generator_->RandBool( 8u ) )
			{
				const int boring_animation= GetAnimation( AnimationId::Boring );
				if( boring_animation >= 0 && int(current_animation_) != boring_animation )
					current_animation_= boring_animation;
				else
					current_animation_= idle_animation;
			}
			else
				current_animation_= idle_animation;
			current_animation_start_time_+= animation_duration;
		}
		break;

	case State::MoveToTarget:
	{
		// Try melee attack.
		if( animation_frame_unwrapped >= frame_count && target_is_alive &&
			distance_for_melee_attack <= description.attack_radius )
		{
			const int animation= SelectMeleeAttackAnimation();
			if( animation >= 0 )
			{
				state_= State::MeleeAttack;
				current_animation_= animation;
				current_animation_start_time_+= animation_duration;
				attack_was_done_= false;
			}
		}
		if( state_ == State::MoveToTarget )
		{
			if( animation_frame_unwrapped >= frame_count )
			{
				if( description.rock >= 0 && target_is_alive &&
					have_right_hand_ && // Monster hold weapon in right hand
					CanSee( map, target->Position() ) )
				{
					state_= State::RemoteAttack;
					current_animation_= GetAnimation( AnimationId::RemoteAttack );
					attack_was_done_= false;
				}
				else
					SelectTarget( map );

				if( target != nullptr && !target_is_alive )
				{
					// If no target - go to Idle state.
					state_= State::Idle;
					current_animation_= GetIdleAnimation();
				}

				current_animation_start_time_+= animation_duration;
			}

			if( state_ == State::MoveToTarget )
				MoveToTarget( last_tick_delta_s );
		}
	}
		break;

	case State::PainShock:
		if( animation_frame_unwrapped >= frame_count )
		{
			state_= State::MoveToTarget;
			SelectTarget( map );
			current_animation_= GetAnimation( AnimationId::Run );
			current_animation_start_time_+= animation_duration;
		}
		break;

	case State::MeleeAttack:
		if( animation_frame_unwrapped >= frame_count )
		{			
			state_= State::MoveToTarget;

			// Try play win animation.
			if( animation_frame_unwrapped >= frame_count &&
				target != nullptr && target->Health() <= 0 )
			{
				const int animation= GetAnimation( AnimationId::Win );
				if( animation >= 0 )
				{
					map.PlayMonsterSound( monster_id, Sound::MonsterSoundId::Win );
					state_= State::Idle;
					current_animation_= animation;
				}
			}
			// Try do new melee attack just after previous melee attack.
			if( state_== State::MoveToTarget &&
				target_is_alive && distance_for_melee_attack <= description.attack_radius )
			{
				const int animation= SelectMeleeAttackAnimation();
				if( animation >= 0 )
				{
					state_= State::MeleeAttack;
					current_animation_= animation;
					attack_was_done_= false;
				}
			}
			if( state_ == State::MoveToTarget )
			{
				if( target_is_alive || target == nullptr )
				{
					SelectTarget( map );
					current_animation_= GetAnimation( AnimationId::Run );
				}
				else
				{
					// If no target - go to Idle state.
					state_= State::Idle;
					current_animation_= GetIdleAnimation();
				}
			}
			current_animation_start_time_+= animation_duration;
		}
		else
		{
			RotateToTarget( last_tick_delta_s );

			if( animation_frame_unwrapped >= frame_count / 2u &&
				!attack_was_done_ &&
				target_is_alive &&
				distance_for_melee_attack <= description.attack_radius  )
			{
				target->Hit(
					description.kick, ( target->Position().xy() - pos_.xy() ), monster_id,
					map,
					target_.monster_id, current_time );

				map.PlayMonsterSound( monster_id, Sound::MonsterSoundId::MeleeAttack );

				attack_was_done_= true;
			}
		}
		break;

	case State::RemoteAttack:
		if( animation_frame_unwrapped >= frame_count )
		{
			// Try play win animation.
			if( target != nullptr && target->Health() <= 0 )
			{
				const int animation= GetAnimation( AnimationId::Win );
				if( animation >= 0 )
				{
					map.PlayMonsterSound( monster_id, Sound::MonsterSoundId::Win );
					state_= State::Idle;
					current_animation_= animation;
				}
			}
			if( state_ == State::RemoteAttack )
			{
				if( target_is_alive || target == nullptr )
				{
					state_= State::MoveToTarget;
					SelectTarget( map );
					current_animation_= GetAnimation( AnimationId::Run );
				}
				else
				{
					// If no target - go to Idle state.
					state_= State::Idle;
					current_animation_= GetIdleAnimation();
				}
			}

			current_animation_start_time_+= animation_duration;
		}
		else
		{
			RotateToTarget( last_tick_delta_s );

			if( animation_frame_unwrapped >= frame_count / 2u &&
				!attack_was_done_ &&
				target != nullptr )
			{
				DoShoot( target->Position(), map, monster_id, current_time );
				attack_was_done_= true;
			}
		}
		break;

	case State::DeathAnimation:
		if( animation_frame_unwrapped >= frame_count )
		{
			state_= State::Dead;
			current_animation_frame_= frame_count - 1u;
		}
		else
			current_animation_frame_= animation_frame_unwrapped;
		break;

	case State::Dead:
		current_animation_frame_= frame_count - 1u; // Last frame of death animation
		break;
	};

	// Set frame for new animation.
	if( state_ != State::Dead )
	{
		const float new_time_delta_s= (current_time - current_animation_start_time_ ).ToSeconds();
		const float new_frame= new_time_delta_s * GameConstants::animations_frames_per_second;
		const unsigned int new_frame_i= static_cast<unsigned int>(new_frame);
		const unsigned int current_animation_frame_count= model.animations[ current_animation_ ].frame_count;
		current_animation_frame_= std::min( new_frame_i, current_animation_frame_count - 1u );
	}
}

void Monster::Hit(
	const int damage,
	const m_Vec2& hit_direction,
	const EntityId opponent_id,
	Map& map,
	const EntityId monster_id,
	const Time current_time)
{
	PC_UNUSED( hit_direction );

	if( state_ != State::DeathAnimation && state_ != State::Dead )
	{
		// If monster hited by other monster - set this monster as target.
		if( opponent_id != monster_id )
		{
			const Map::MonstersContainer& map_monsters= map.GetMonsters();
			const auto it= map_monsters.find( opponent_id );
			if( it != map_monsters.end() )
			{
				PC_ASSERT( it->second );
				target_.monster_id= it->first;
				target_.monster= it->second;
				target_.position= it->second->Position();
				target_.have_position= true;
			}
		}

		const bool is_final_boss= IsFinalBoss();
		const bool is_boss= IsBoss();
		const bool is_environment_damage= opponent_id == 0u;

		if( is_final_boss )
		{} // Final boss is immortal.
		else if( is_boss )
		{
			// Bosses can receive damage only from environment.
			if( is_environment_damage )
				health_-= damage;
		}
		else
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
			const int c_fragmentation_health_limit= -20;

			// Select animation.
			// Select random death animation, if there are 2 possible death animations.
			int animation, death_animations[2];
			death_animations[0]= GetAnimation( AnimationId::Death0 );
			death_animations[1]= GetAnimation( AnimationId::Death1 );
			if( death_animations[0] >= 0 && death_animations[1] >= 0 )
				animation= random_generator_->RandBool() ? death_animations[0] : death_animations[1];
			else if( death_animations[0] >= 0 )
				animation= death_animations[0];
			else
				animation= death_animations[1];

			if( animation < 0 || is_boss || health_ <= c_fragmentation_health_limit )
			{
				// Fragment monster body.
				state_= State::Dead;
				fragmented_= true;

				// Produce simple gibs.
				// TODO - make more complex gibs.
				const m_Vec2 z_min_max= GetZMinMax();
				map.AddParticleEffect(
					pos_ + m_Vec3( 0.0f, 0.0f, 0.5f * ( z_min_max.x + z_min_max.y ) ),
					static_cast<ParticleEffect>( static_cast<unsigned char>(ParticleEffect::FirstBlowEffect) + 72u ) );

				SpawnBodyPart( map, BodyPartSubmodelId::Head );
			}
			else
			{
				// If we not fragmented - spawn head with some chance.
				if( random_generator_->RandBool() )
				{
					have_head_= false;
					SpawnBodyPart( map, BodyPartSubmodelId::Head );
				}

				state_= State::DeathAnimation;
				current_animation_= static_cast<unsigned int>(animation);
			}

			current_animation_start_time_= current_time;

			map.PlayMonsterSound( monster_id, Sound::MonsterSoundId::Death );

			BackpackPtr backpack;
			if( is_boss )
			{
				// Bosses drops packs with keys.
				backpack.reset( new Backpack );
				backpack->red_key= backpack->green_key= backpack->blue_key= true;
			}
			else if( monster_id_ == 3u || monster_id_ == 6u )
			{
				// Wing-Man and MongF drop shotgun shells and armor.
				backpack.reset( new Backpack );
				backpack->ammo[1u]= 5u;
				backpack->armor= 2u;
			}
			else if( monster_id_ == 7u )
			{
				// Faust drops armor and grenades.
				backpack.reset( new Backpack );
				backpack->ammo[5u]= 3u;
				backpack->armor= 2u;
			}

			if( backpack != nullptr )
			{
				const m_Vec2 z_minmax= GetZMinMax();
				backpack->pos= pos_;
				backpack->pos.z+= ( z_minmax.x + z_minmax.y ) * 0.5f;
				map.SpawnBackpack( std::move( backpack ) );
			}
		}
	}
}

void Monster::TryWarn(
	const m_Vec3& pos,
	Map& map,
	const EntityId monster_id,
	const Time current_time )
{
	if( state_ != State::Idle )
		return;

	if( ( pos.xy() - pos_.xy() ).SquareLength() < 10.0f )
		return;

	if( !map.CanSee(
			m_Vec3( pos_.xy(), GameConstants::walls_height * 0.5f ),
			m_Vec3( pos .xy(), GameConstants::walls_height * 0.5f ) ) )
		return;

	map.PlayMonsterSound( monster_id, Sound::MonsterSoundId::Alarmed );
	state_= State::MoveToTarget;
	current_animation_= GetAnimation( AnimationId::Run );
	current_animation_start_time_ = current_time;

	target_.have_position= true;
	target_.position= pos;
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
	angle_= angle + Constants::half_pi;
	vertical_speed_= 0.0f;
}

bool Monster::IsFullyDead() const
{
	return state_ == State::Dead;
}

unsigned char Monster::GetColor() const
{
	return 0u;
}

bool Monster::IsBoss() const
{
	// Bosses have hardcoded id.
	// TODO - check, how bosses works in original game.
	static const unsigned char c_bosses_id[]=
	{
		9u, 18u, 19u, 20u
	};

	for( const unsigned char boss_id : c_bosses_id )
		if( monster_id_ == boss_id )
			return true;

	return false;
}

bool Monster::IsFinalBoss() const
{
	return monster_id_ == 20u;
}

bool Monster::CanSee( const Map& map, const m_Vec3& pos ) const
{
	return map.CanSee( pos_ + g_see_point_delta, pos + g_see_point_delta );
}

unsigned int Monster::GetIdleAnimation() const
{
	return GetAnyAnimation( { AnimationId::Idle0, AnimationId::Idle1, AnimationId::Run } );
}

void Monster::DoShoot( const m_Vec3& target_pos, Map& map, const EntityId monster_id, const Time current_time )
{
	const GameResources::MonsterDescription& description= game_resources_->monsters_description[ monster_id_ ];
	const ShootInfo& shoot_info= g_shoot_info[ monster_id_ ];

	m_Mat4 rot_mat;
	rot_mat.RotateZ( angle_);

	m_Vec3 avg_shoot_pos( 0.0f, 0.0f, 0.0f );
	for( unsigned int i= 0u; i < shoot_info.count; i++ )
		avg_shoot_pos+= shoot_info.positions[i] * rot_mat;
	avg_shoot_pos/= float(shoot_info.count);

	m_Vec3 dir= target_pos + g_target_shoot_point_delta - ( pos_ + avg_shoot_pos );
	dir.Normalize();

	for( unsigned int i= 0u; i < shoot_info.count; i++ )
	{
		const m_Vec3 base_shoot_pos= shoot_info.positions[i] * rot_mat;
		const m_Vec3 shoot_pos= pos_ + base_shoot_pos;

		PC_ASSERT( description.rock >= 0 );
		map.Shoot( monster_id, description.rock, shoot_pos, dir, current_time );
		map.PlayMonsterSound( monster_id, Sound::MonsterSoundId::RemoteAttack );
	}
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
	if( !target_.have_position )
		return;

	const m_Vec2 vec_to_target= target_.position.xy() - pos_.xy();
	const float vec_to_target_length= vec_to_target.Length();

	// Nothing to do, we are on target
	if( vec_to_target_length == 0.0f )
		return;

	const GameResources::MonsterDescription& monster_description= game_resources_->monsters_description[ monster_id_ ];

	const float distance_delta= time_delta_s * float( monster_description.speed ) / 10.0f;

	if( distance_delta >= vec_to_target_length )
		target_.have_position= false; // Reached

	pos_.x+= std::cos(angle_) * distance_delta;
	pos_.y+= std::sin(angle_) * distance_delta;

	RotateToTarget( time_delta_s );
}

void Monster::RotateToTarget( float time_delta_s )
{
	if( !target_.have_position )
		return;

	const m_Vec2 vec_to_target= target_.position.xy() - pos_.xy();
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

bool Monster::SelectTarget( const Map& map )
{
	{
		// Clear current target, if needed.
		const MonsterBasePtr target= target_.monster.lock();
		if( target != nullptr && target->Health() <= 0 )
		{
			target_.monster_id= 0u;
			target_.monster= MonsterBaseWeakPtr();
			target_.have_position= false;
		}

		// Already have target - do not select.
		if( target != nullptr && target->Health() > 0 && target_.have_position )
			return true;
	}

	const float c_half_view_angle= Constants::half_pi * 0.75f;
	const float c_half_view_angle_cos= std::cos( c_half_view_angle );
	const m_Vec2 view_dir( std::cos(angle_), std::sin(angle_) );

	float nearest_player_distance= Constants::max_float;
	const Map::PlayersContainer::value_type* nearest_player= nullptr;

	const Map::PlayersContainer& players= map.GetPlayers();
	for( const Map::PlayersContainer::value_type& player_value : players )
	{
		PC_ASSERT( player_value.second != nullptr );
		const Player& player= *player_value.second;

		if( player.Health() <= 0 )
			continue;

		const m_Vec2 dir_to_player= player.Position().xy() - Position().xy();
		const float distance_to_player= dir_to_player.Length();
		if( distance_to_player == 0.0f )
			continue;

		if( distance_to_player >= nearest_player_distance )
			continue;

		if( state_ == State::Idle )
		{
			// Monsters in Idle state have no back eyes.
			const float angle_cos= ( dir_to_player * view_dir ) / distance_to_player;
			if( angle_cos < c_half_view_angle_cos )
				continue;
		}

		if( CanSee( map, player.Position() ) )
		{
			nearest_player_distance= distance_to_player;
			nearest_player= &player_value;
		}
	}

	if( nearest_player != nullptr )
	{
		target_.monster_id= nearest_player->first;
		target_.monster= nearest_player->second;
		target_.position= nearest_player->second->Position();
		target_.have_position= true;
	}

	return target_.have_position;
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
