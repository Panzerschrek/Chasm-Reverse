#include <cstring>

#include <matrix.hpp>

#include "../map_loader.hpp"
#include "../math_utils.hpp"
#include "../messages_sender.hpp"
#include "../sound/sound_id.hpp"
#include "map.hpp"

#include "player.hpp"

namespace PanzerChasm
{

static const float g_damage_acceleration_scale= 1.0f / 12.0f;
static const float g_mega_destroyer_recoil_speed= 10.0f;

Player::Player( const GameResourcesConstPtr& game_resources, const Time current_time )
	: MonsterBase( game_resources, 0u, m_Vec3( 0.0f, 0.0f, 0.0f ), 0.0f )
	, spawn_time_( current_time )
	, speed_( 0.0f, 0.0f, 0.0f )
	, on_floor_(false)
	, noclip_(false)
	, god_mode_(false)
	, teleported_(true)
	, armor_(0)
	, last_state_change_time_( current_time )
	, last_shoot_time_( current_time )
	, weapon_animation_state_change_time_( current_time )
	, last_pain_sound_time_( current_time )
	, last_step_sound_time_( current_time )
{
	PC_ASSERT( game_resources_ != nullptr );

	for( unsigned int i= 0u; i < GameConstants::weapon_count; i++ )
	{
		ammo_[i]= 0u;
		have_weapon_[i]= false;
	}

	// Give rifle
	ammo_[0]= game_resources_->weapons_description[0].limit;
	have_weapon_[0]= true;

	// give mines fake weapon.
	have_weapon_[ GameConstants::mine_weapon_number ]= true;
}

Player::~Player()
{}

void Player::Tick(
	Map& map,
	const EntityId monster_id,
	const Time current_time,
	const Time last_tick_delta )
{
	teleported_= false;

	// Process invisibility
	if( has_invisibility_ )
	{
		const float invisiblity_time_s= ( current_time - invisibility_take_time_ ).ToSeconds();
		if( invisiblity_time_s > GameConstants::invisibility_time_s )
		{
			has_invisibility_= false;
			inviible_in_this_moment_= false;
		}
		else if( invisiblity_time_s >= GameConstants::invisibility_flashing_start_time_s )
		{
			inviible_in_this_moment_=
				( static_cast<int>( 2.0f * ( invisiblity_time_s - GameConstants::invisibility_flashing_start_time_s ) ) & 1 ) != 0;
		}
		else
			inviible_in_this_moment_= true;
	}
	else
		inviible_in_this_moment_= false;

	// Process animations.
	if( state_ == State::Alive )
	{
		if( mevement_acceleration_ > 0.0f )
		{
			float angle_diff= movement_direction_ - angle_;
			if( angle_diff > +Constants::pi )
				angle_diff-= Constants::two_pi;
			if( angle_diff < -Constants::pi )
				angle_diff+= Constants::two_pi;

			if( std::abs(angle_diff) <= Constants::half_pi )
				current_animation_= GetAnimation( AnimationId::Run );
			else
				current_animation_= GetAnimation( AnimationId::MeleeAttackLeftHand ); // TODO - select backwalk animation
		}
		else
			current_animation_= GetAnimation( AnimationId::Idle0 );

		const float frame= ( current_time - spawn_time_ ).ToSeconds() * GameConstants::animations_frames_per_second;
		current_animation_frame_=
			static_cast<unsigned int>( std::round( frame ) ) %
			game_resources_->monsters_models[0].animations[ current_animation_ ].frame_count;
	}
	if( state_ == State::DeathAnimation )
	{
		current_animation_= GetAnimation( AnimationId::Respawn ); // For player "respawn" animation is death animation.

		const float animation_time_delta_s= ( current_time - last_state_change_time_ ).ToSeconds();
		const unsigned int frame=
			static_cast<unsigned int>( std::round( animation_time_delta_s * GameConstants::animations_frames_per_second ) );

		if( frame >= game_resources_->monsters_models[0].animations[ current_animation_ ].frame_count )
		{
			last_state_change_time_= current_time;
			state_= State::Dead;
		}
		else
			current_animation_frame_= frame;
	}
	if( state_ == State::Dead )
	{
		// Use last frame of previous death animation.
		current_animation_frame_= game_resources_->monsters_models[0].animations[ current_animation_ ].frame_count - 1u;
	}

	if( state_ != State::Alive )
		return;

	// Move.
	const bool jumped= Move( last_tick_delta );

	// Play move/jump sounds.
	if( jumped )
		map.PlayMonsterLinkedSound( monster_id, Sound::SoundId::Jump );
	else if( on_floor_ &&
		mevement_acceleration_ > 0.0f &&
		( current_time - last_step_sound_time_ ).ToSeconds() > 0.4f )
	{
		last_step_sound_time_= current_time;
		map.PlayMonsterLinkedSound( monster_id, Sound::SoundId::StepRun );
	}

	// Make some iterations of weapon logic.
	bool switch_step_done= false;
	for( unsigned int iter= 0u; iter < 3u; iter++ )
	{
		// Try start switching
		if( weapon_state_ == WeaponState::Idle )
		{
			if( current_weapon_index_ != requested_weapon_index_ )
				weapon_state_ = WeaponState::Switching;
		}
		// Try end reloading
		if( weapon_state_ == WeaponState::Reloading )
		{
			const float time_sinse_last_shot_s= ( current_time - last_shoot_time_ ).ToSeconds();
			const float reloading_time= float( game_resources_->weapons_description[ current_weapon_index_ ].reloading_time ) / 128.0f;
			if( time_sinse_last_shot_s >= reloading_time )
				weapon_state_= WeaponState::Idle;
		}
		// Switch
		if( weapon_state_ == WeaponState::Switching && !switch_step_done )
		{
			const float delta= 2.0f * last_tick_delta.ToSeconds() / GameConstants::weapon_switch_time_s;

			if( current_weapon_index_ != requested_weapon_index_ )
			{
				if( weapon_switch_stage_ > 0.0f )
					weapon_switch_stage_= std::max( weapon_switch_stage_ - delta, 0.0f );

				if( weapon_switch_stage_ <= 0.0f )
					current_weapon_index_= requested_weapon_index_;
			}
			else if( weapon_switch_stage_ < 1.0f )
				weapon_switch_stage_= std::min( weapon_switch_stage_ + delta, 1.0f );

			if( weapon_switch_stage_ >= 1.0f )
				weapon_state_= WeaponState::Idle;

			switch_step_done= true;
		}
		// Try shoot
		if( weapon_state_ == WeaponState::Idle && shoot_pressed_ &&
			( ammo_[ current_weapon_index_ ] > 0u || current_weapon_index_ == 0 ) )
		{
			if( current_weapon_index_ == GameConstants::mine_weapon_number )
				map.PlantMine( monster_id, pos_, current_time );
			else
			{
				const GameResources::WeaponDescription& description= game_resources_->weapons_description[ current_weapon_index_ ];

				const m_Vec3 view_vec( 0.0f, 1.0f, 0.0f );

				m_Mat4 x_rotate, z_rotate, rotate;
				x_rotate.RotateX( view_angle_x_ );
				z_rotate.RotateZ( view_angle_z_ );
				rotate= x_rotate * z_rotate;

				const m_Vec3 view_vec_rotated= view_vec * rotate;

				// TODO - check and calibrate
				const m_Vec3 shoot_point_delta(
					1.0f / 16.0f,
					0.0f,
					-float(description.r_z0) / 2048.0f );

				const m_Vec3 final_shoot_pos=
					pos_ + m_Vec3( 0.0f, 0.0f, GameConstants::player_eyes_level ) + shoot_point_delta * rotate;

				for( unsigned int i= 0u; i < static_cast<unsigned int>(description.r_count); i++ )
				{
					m_Vec3 final_view_dir_vec;
					if( description.r_count > 1 && random_generator_ != nullptr )
					{
						final_view_dir_vec= view_vec_rotated + random_generator_->RandPointInSphere( 1.0f / 24.0f );
						final_view_dir_vec.Normalize();
					}
					else
						final_view_dir_vec= view_vec_rotated;

					map.Shoot(
						monster_id, description.r_type,
						final_shoot_pos, final_view_dir_vec,
						current_time );
				}

				if( current_weapon_index_ == GameConstants::mega_destroyer_weapon_number )
					speed_+=
						(-g_mega_destroyer_recoil_speed) *
						m_Vec3( view_vec_rotated.xy(), 0.0f );

				map.PlayMonsterLinkedSound( monster_id, Sound::SoundId::FirstWeaponFire + current_weapon_index_ );
			}

			if( current_weapon_index_ != 0u )
				ammo_[ current_weapon_index_ ]--;

			last_shoot_time_= current_time;
			weapon_state_= WeaponState::Reloading;
		}

	}

	{ // Weapon anination
		bool use_idle_animation= false;
		if( weapon_state_ == WeaponState::Reloading )
		{
			const float weapon_time_s= ( current_time - last_shoot_time_ ).ToSeconds();
			const Model& model= game_resources_->weapons_models[ current_weapon_index_ ];

			current_weapon_animation_= 1u;
			const unsigned int frame= static_cast<unsigned int>( std::round( weapon_time_s * GameConstants::weapons_animations_frames_per_second ) );

			const unsigned int frame_count= model.animations[ current_weapon_animation_ ].frame_count;
			if( frame >= frame_count )
			{
				weapon_animation_state_change_time_= current_time;
				use_idle_animation= true;
			}
			else
				current_weapon_animation_frame_= frame;
		}
		if( weapon_state_ != WeaponState::Reloading || use_idle_animation )
		{
			const float weapon_time_s= ( current_time - weapon_animation_state_change_time_ ).ToSeconds();
			const Model& model= game_resources_->weapons_models[ current_weapon_index_ ];

			const float frame= weapon_time_s * GameConstants::weapons_animations_frames_per_second;

			current_weapon_animation_= weapon_state_ == WeaponState::Reloading ? 1u : 0u;
			current_weapon_animation_frame_=
				static_cast<unsigned int>( std::round( frame ) ) %
				model.animations[ current_weapon_animation_ ].frame_count;
		}
	}
}

void Player::Hit(
	const int damage,
	const m_Vec2& hit_direction,
	const EntityId opponent_id,
	Map& map,
	const EntityId monster_id,
	const Time current_time )
{
	PC_UNUSED( opponent_id );
	PC_UNUSED( current_time );

	if( state_ != State::Alive )
		return;

	// TODO - in original game damage is bigger or smaller, sometimes. Know, why.

	// Armor absorbess all damage and gives 1/4 of absorbed damage to health.
	int armor_damage= damage;
	if( armor_damage > armor_ )
		armor_damage= armor_;

	int health_damage= ( damage - armor_damage ) + armor_damage / 4;

	if( !god_mode_ )
	{
		armor_-= armor_damage;
		health_-= health_damage;
	}

	if( health_ <= 0 )
	{
		// Player is dead now.
		state_= State::DeathAnimation;
		last_state_change_time_= current_time;
		map.PlayMonsterSound( monster_id, Sound::PlayerMonsterSoundId::Death );

		if( opponent_id != monster_id ) // If it is not suicide.
		{
			const Map::PlayersContainer& players= map.GetPlayers();
			const auto opponent_it= players.find( opponent_id );
			if( opponent_it != players.end() )
			{
				Player& opponent= *opponent_it->second;
				opponent.frags_++;
			}
		}
	}
	else if( ( current_time - last_pain_sound_time_ ).ToSeconds() >= 0.5f )
	{
		last_pain_sound_time_= current_time;
		map.PlayMonsterSound(
			monster_id,
			random_generator_->RandBool() ? Sound::PlayerMonsterSoundId::Pain0 : Sound::PlayerMonsterSoundId::Pain1 );
	}

	// Push player, but only if hit direction length is nonzero.
	// Zero it is for damage from environment, for example.

	const float hit_direction_square_length= hit_direction.SquareLength();
	if( hit_direction_square_length != 0.0f )
	{
		const m_Vec2 hit_direction_normalized= hit_direction / std::sqrt( hit_direction_square_length );

		const float c_damage_acceleration_scale= 1.0f / 12.0f;

		const m_Vec2 speed_delta=
			float( std::min( damage, GameConstants::player_max_health ) ) *
			c_damage_acceleration_scale *
			hit_direction_normalized;

		speed_+= m_Vec3( speed_delta, 0.0f );
	}

	// Add blood falshes.
	if( health_damage > 0 )
	{
		// Flash intensity depends on dagage.
		fullscreen_blend_messages_.emplace_back();
		fullscreen_blend_messages_.back().color_index= 63u;
		fullscreen_blend_messages_.back().intensity= std::min( 255u, static_cast<unsigned int>(health_damage + 5) * 5u );
	}
}

void Player::TryWarn(
	const m_Vec3& pos,
	Map& map,
	const EntityId monster_id,
	const Time current_time )
{
	PC_UNUSED( pos );
	PC_UNUSED( map );
	PC_UNUSED( monster_id );
	PC_UNUSED( current_time );
}

void Player::ClampSpeed( const m_Vec3& clamp_surface_normal )
{
	const float projection= clamp_surface_normal * speed_;
	if( projection < 0.0f )
		speed_-= clamp_surface_normal * projection;
}

void Player::SetOnFloor( const bool on_floor )
{
	on_floor_= on_floor;
	if( on_floor_ && speed_.z < 0.0f )
		speed_.z= 0.0f;
}

void Player::Teleport( const m_Vec3& pos, const float angle )
{
	teleported_= true;
	pos_= pos;
	angle_= angle;
	speed_.x= speed_.y= speed_.z= 0.0f;
}

bool Player::IsFullyDead() const
{
	return state_ == State::Dead;
}

unsigned char Player::GetColor() const
{
	return color_;
}

bool Player::IsInvisible() const
{
	return state_ != State::Dead && has_invisibility_;
}

void Player::BuildStateMessage( Messages::MonsterState& out_message ) const
{
	PositionToMessagePosition( pos_, out_message.xyz );
	out_message.angle= AngleToMessageAngle( angle_ );
	out_message.monster_type= 0u;
	out_message.body_parts_mask= GetBodyPartsMask();
	out_message.animation= CurrentAnimation();
	out_message.animation_frame= CurrentAnimationFrame();
	out_message.is_fully_dead= IsFullyDead();
	out_message.is_invisible= inviible_in_this_moment_;
	out_message.color= GetColor();
}

void Player::SetRandomGenerator( const LongRandPtr& random_generator )
{
	random_generator_= random_generator;
}

bool Player::TryActivateProcedure( const unsigned int proc_number, const Time current_time )
{
	if( proc_number == last_activated_procedure_ )
	{
		// Activate again only after delay.
		if( current_time - last_activated_procedure_activation_time_ <= Time::FromSeconds(2) )
			return false;
	}

	last_activated_procedure_= proc_number;
	last_activated_procedure_activation_time_= current_time;
	return true;
}

void Player::ResetActivatedProcedure()
{
	last_activated_procedure_= 0u;
	last_activated_procedure_activation_time_= Time::FromSeconds(0);
}

bool Player::TryPickupItem( const unsigned int item_id, const Time current_time )
{
	if( item_id >=  game_resources_->items_description.size() )
		return false;

	const ACode a_code= static_cast<ACode>( game_resources_->items_description[ item_id ].a_code );

	if( a_code >= ACode::Weapon_First && a_code <= ACode::Weapon_Last )
	{
		const unsigned int weapon_index=
			static_cast<unsigned int>( a_code ) - static_cast<unsigned int>( ACode::Weapon_First );

		const GameResources::WeaponDescription& weapon_description= game_resources_->weapons_description[ weapon_index ];

		if( have_weapon_[ weapon_index ] &&
			ammo_[ weapon_index ] >= static_cast<unsigned int>( weapon_description.limit ) )
			return false;

		have_weapon_[ weapon_index ]= true;

		unsigned int new_ammo_count= ammo_[ weapon_index ] + static_cast<unsigned int>( weapon_description.start );
		new_ammo_count= std::min( new_ammo_count, static_cast<unsigned int>( weapon_description.limit ) );
		ammo_[ weapon_index ]= new_ammo_count;

		GenItemPickupMessage( item_id );
		AddItemPickupFlash();
		return true;
	}
	else if( a_code >= ACode::Ammo_First && a_code <= ACode::Ammo_Last )
	{
		const unsigned int weapon_index=
			static_cast<unsigned int>( a_code ) / 10u - static_cast<unsigned int>( ACode::Weapon_First );

		const unsigned int ammo_portion_size= static_cast<unsigned int>( a_code ) % 10u;

		const GameResources::WeaponDescription& weapon_description= game_resources_->weapons_description[ weapon_index ];

		if( ammo_[ weapon_index ] >= static_cast<unsigned int>( weapon_description.limit ) )
			return false;

		unsigned int new_ammo_count=
			ammo_[ weapon_index ] +
			static_cast<unsigned int>( weapon_description.d_am ) * ammo_portion_size;

		new_ammo_count= std::min( new_ammo_count, static_cast<unsigned int>( weapon_description.limit ) );
		ammo_[ weapon_index ]= new_ammo_count;

		GenItemPickupMessage( item_id );
		AddItemPickupFlash();
		return true;
	}
	else if( a_code == ACode::Item_Invisibility )
	{
		if( has_invisibility_ )
			invisibility_take_time_+= Time::FromSeconds( GameConstants::invisibility_time_s );
		else
		{
			has_invisibility_= true;
			invisibility_take_time_= current_time;
		}
		return true;
	}
	else if( a_code == ACode::Item_Life )
	{
		if( health_ < GameConstants::player_nominal_health )
		{
			health_= std::min( health_ + 20, GameConstants::player_nominal_health );
			GenItemPickupMessage( item_id );
			AddItemPickupFlash();
			return true;
		}
		return false;
	}
	else if( a_code == ACode::Item_BigLife )
	{
		if( health_ < GameConstants::player_max_health )
		{
			health_= std::min( health_ + 100, GameConstants::player_max_health );

			GenItemPickupMessage( item_id );
			AddItemPickupFlash();
			return true;
		}
		return false;
	}
	else if( a_code == ACode::Item_Armor )
	{
		if( armor_ < GameConstants::player_max_armor )
		{
			armor_= std::min( armor_ + 200, GameConstants::player_max_armor );

			GenItemPickupMessage( item_id );
			AddItemPickupFlash();
			return true;
		}
		return false;
	}
	else if( a_code == ACode::Item_Helmet )
	{
		if( armor_ < GameConstants::player_max_armor )
		{
			armor_= std::min( armor_ + 100, GameConstants::player_max_armor );

			GenItemPickupMessage( item_id );
			AddItemPickupFlash();
			return true;
		}
		return false;
	}

	return false;
}

bool Player::TryPickupBackpack( const Backpack& backpack )
{
	for( unsigned int i= 0u; i < GameConstants::weapon_count; i++ )
	{
		have_weapon_[i]= have_weapon_[i] || backpack.weapon[i];
		ammo_[i]= std::min( int( ammo_[i] + backpack.ammo[i] ), game_resources_->weapons_description[i].limit );
	}

	have_red_key_  = have_red_key_   || backpack.red_key  ;
	have_green_key_= have_green_key_ || backpack.green_key;
	have_blue_key_ = have_blue_key_  || backpack.blue_key ;

	armor_+= backpack.armor;
	if( armor_ > GameConstants::player_max_armor ) armor_= GameConstants::player_max_armor;

	// TODO - return if really something new picked.
	AddItemPickupFlash();
	return true;
}

void Player::BuildPositionMessage( Messages::PlayerPosition& out_position_message ) const
{
	PositionToMessagePosition( pos_, out_position_message.xyz );
	out_position_message.speed= CoordToMessageCoord( speed_.xy().Length() );
}

void Player::BuildStateMessage( Messages::PlayerState& out_state_message ) const
{
	for( unsigned int i= 0u; i < GameConstants::weapon_count; i++ )
		out_state_message.ammo[i]= ammo_[i];

	out_state_message.health= std::max( 0, health_ );
	out_state_message.armor= armor_;

	out_state_message.keys_mask= 0u;
	if( have_red_key_   ) out_state_message.keys_mask|= 1u;
	if( have_green_key_ ) out_state_message.keys_mask|= 2u;
	if( have_blue_key_  ) out_state_message.keys_mask|= 4u;

	out_state_message.weapons_mask= 0u;
	for( unsigned int i= 0u; i < GameConstants::weapon_count; i++ )
		if( have_weapon_[i] )
			out_state_message.weapons_mask|= 1 << i;

	out_state_message.is_invisible= inviible_in_this_moment_;
}

void Player::BuildWeaponMessage( Messages::PlayerWeapon& out_weapon_message ) const
{
	out_weapon_message.current_weapon_index_= current_weapon_index_;
	out_weapon_message.animation= current_weapon_animation_;
	out_weapon_message.animation_frame= current_weapon_animation_frame_;

	out_weapon_message.switch_stage= static_cast<unsigned int>( weapon_switch_stage_ * 254.9f );
}

bool Player::BuildSpawnMessage( Messages::PlayerSpawn& out_spawn_message, const bool force ) const
{
	if( !teleported_ && !force )
		return false;

	PositionToMessagePosition( pos_, out_spawn_message.xyz );
	out_spawn_message.direction= AngleToMessageAngle( angle_ );

	return true;
}

void Player::SendInternalMessages( MessagesSender& messages_sender )
{
	for( const auto& message : pickup_messages_ )
		messages_sender.SendUnreliableMessage( message );
	for( const auto& message : fullscreen_blend_messages_ )
		messages_sender.SendUnreliableMessage( message );

	pickup_messages_.clear();
	fullscreen_blend_messages_.clear();
}

void Player::OnMapChange()
{
	have_red_key_= have_green_key_= have_blue_key_= false;
	last_activated_procedure_= ~0u;

	has_invisibility_= false;
	inviible_in_this_moment_= false;
}

void Player::UpdateMovement( const Messages::PlayerMove& move_message )
{
	if( state_ != State::Alive )
		return;

	angle_= MessageAngleToAngle( move_message.view_direction );
	movement_direction_= MessageAngleToAngle( move_message.move_direction );
	mevement_acceleration_= float(move_message.acceleration) / 255.0f;
	jump_pessed_= move_message.jump_pressed;

	if( have_weapon_[ move_message.weapon_index ] )
	{
		requested_weapon_index_= move_message.weapon_index;
		if( requested_weapon_index_ >= GameConstants::weapon_count )
			requested_weapon_index_= 0u;
	}

	view_angle_x_= MessageAngleToAngle( move_message.view_dir_angle_x );
	view_angle_z_= MessageAngleToAngle( move_message.view_dir_angle_z );
	shoot_pressed_= move_message.shoot_pressed;

	color_= move_message.color;
}

void Player::SetNoclip( const bool noclip )
{
	noclip_= noclip;
}

bool Player::IsNoclip() const
{
	return noclip_;
}

void Player::SetGodMode( const bool god_mode )
{
	god_mode_= god_mode;
	if( god_mode_ )
	{
		health_= GameConstants::player_max_health;
		armor_= GameConstants::player_max_armor;
	}
}

void Player::GiveWeapon()
{
	for( unsigned int w= 0u; w < GameConstants::weapon_count; w++ )
	{
		have_weapon_[w]= true;
		ammo_[w]= game_resources_->weapons_description[w].limit;
	}
}

void Player::GiveAmmo()
{
	for( unsigned int w= 0u; w < GameConstants::weapon_count; w++ )
		ammo_[w]= game_resources_->weapons_description[w].limit;
}

void Player::GiveArmor()
{
	armor_= GameConstants::player_max_armor;
}

void Player::GiveRedKey()
{
	have_red_key_= true;
}

void Player::GiveGreenKey()
{
	have_green_key_= true;
}

void Player::GiveBlueKey()
{
	have_blue_key_= true;
}

void Player::GiveAllKeys()
{
	GiveRedKey();
	GiveGreenKey();
	GiveBlueKey();
}

bool Player::HaveRedKey() const
{
	return have_red_key_;
}

bool Player::HaveGreenKey() const
{
	return have_green_key_;
}

bool Player::HaveBlueKey() const
{
	return have_blue_key_;
}

unsigned int Player::CurrentWeaponIndex() const
{
	return current_weapon_index_;
}

void Player::SetFrags( const unsigned int frags )
{
	frags_= frags;
}

unsigned int Player::GetFrags() const
{
	return frags_;
}

bool Player::Move( const Time time_delta )
{
	const float time_delta_s= time_delta.ToSeconds();

	// TODO - calibrate this
	const float c_acceleration= 40.0f;
	const float c_deceleration= 20.0f;
	const float c_jump_speed_delta= 2.9f;

	const float speed_delta= time_delta_s * mevement_acceleration_ * c_acceleration;
	const float deceleration_speed_delta= time_delta_s * c_deceleration;

	// Accelerate
	m_Vec2 acceleration( 0.0f, 0.0f );
	if( state_ == State::Alive )
	{
		acceleration.x= std::cos( movement_direction_ ) * speed_delta;
		acceleration.y= std::sin( movement_direction_ ) * speed_delta;
	}

	// Decelerate
	const float new_speed_length= speed_.xy().Length();
	if( new_speed_length >= deceleration_speed_delta )
	{
		const float k= ( new_speed_length - deceleration_speed_delta ) / new_speed_length;
		speed_.x*= k;
		speed_.y*= k;
	}
	else
		speed_.x= speed_.y= 0.0f;

	const m_Vec2 current_speed_xy= speed_.xy();
	const float acceleration_projection_to_current_speed= acceleration * current_speed_xy;
	if( acceleration_projection_to_current_speed > 0.0f )
	{
		const float max_square_speed= GameConstants::player_max_speed * GameConstants::player_max_speed;

		const float current_speed_square_length= current_speed_xy.SquareLength();
		const m_Vec2 acceleration_projection= current_speed_xy * ( acceleration_projection_to_current_speed / current_speed_square_length );
		const m_Vec2 acceleration_orthogonal= acceleration - acceleration_projection;

		// If speed greater, then maximal speed by player, just add only orthogonal to current speed aceleration part.
		if( current_speed_square_length >= max_square_speed )
		{
			speed_.x+= acceleration_orthogonal.x;
			speed_.y+= acceleration_orthogonal.y;
		}
		else
		{
			// Extend current speed as much, as can and add orthogonal ecceleration component.
			m_Vec2 speed_plus_acceleration_projection= current_speed_xy + acceleration_projection;
			const float speed_plus_acceleration_projection_squar_length= speed_plus_acceleration_projection.SquareLength();
			if( speed_plus_acceleration_projection_squar_length > max_square_speed )
				speed_plus_acceleration_projection*=
					GameConstants::player_max_speed / std::sqrt( speed_plus_acceleration_projection_squar_length );

			speed_.x= speed_plus_acceleration_projection.x + acceleration_orthogonal.x;
			speed_.y= speed_plus_acceleration_projection.y + acceleration_orthogonal.y;
		}
	}
	else
	{
		speed_.x+= acceleration.x;
		speed_.y+= acceleration.y;
	}

	// If speed is veery hight - clamp it.
	const float new_speed_square_length= speed_.xy().SquareLength();
	if( new_speed_square_length > GameConstants::player_max_absolute_speed * GameConstants::player_max_absolute_speed )
	{
		const float k= GameConstants::player_max_absolute_speed / std::sqrt( new_speed_square_length );
		speed_.x*= k;
		speed_.y*= k;
	}

	// Fall down
	speed_.z+= GameConstants::vertical_acceleration * time_delta_s;

	bool jumped= false;

	// Jump
	if( state_ == State::Alive )
	{
		if( jump_pessed_ && noclip_ )
			speed_.z-= 2.0f * GameConstants::vertical_acceleration * time_delta_s;
		else if( jump_pessed_ && on_floor_ && speed_.z <= 0.0f )
		{
			jumped= true;
			speed_.z+= c_jump_speed_delta;
		}
	}

	// Clamp vertical speed
	if( std::abs( speed_.z ) > GameConstants::max_vertical_speed )
		speed_.z*= GameConstants::max_vertical_speed / std::abs( speed_.z );

	pos_+= speed_ * time_delta_s;

	if( noclip_ && pos_.z < 0.0f )
	{
		pos_.z= 0.0f;
		speed_.z= 0.0f;
	}
	return jumped;
}

void Player::GenItemPickupMessage( const unsigned char item_id )
{
	pickup_messages_.emplace_back();
	pickup_messages_.back().item_id= item_id;
}

void Player::AddItemPickupFlash()
{
	// Add green flash.
	fullscreen_blend_messages_.emplace_back();
	fullscreen_blend_messages_.back().color_index= 15u * 16u + 8u;
	fullscreen_blend_messages_.back().intensity= 48u;
}

} // namespace PanzerChasm
