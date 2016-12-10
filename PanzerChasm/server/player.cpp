#include <cstring>

#include <matrix.hpp>

#include "../map_loader.hpp"
#include "./math_utils.hpp"
#include "map.hpp"

#include "player.hpp"

namespace PanzerChasm
{

Player::Player( const GameResourcesConstPtr& game_resources, const Time current_time )
	: MonsterBase( game_resources, 0u, m_Vec3( 0.0f, 0.0f, 0.0f ), 0.0f )
	, spawn_time_( current_time )
	, speed_( 0.0f, 0.0f, 0.0f )
	, on_floor_(false)
	, noclip_(false)
	, health_(100)
	, armor_(0)
	, last_shoot_time_( current_time )
	, weapon_animation_state_change_time_( current_time )
{
	PC_ASSERT( game_resources_ != nullptr );

	for( unsigned int i= 0u; i < GameConstants::weapon_count; i++ )
	{
		ammo_[i]= 0u;
		have_weapon_[i]= false;
	}

	have_weapon_[0]= true;
}

Player::~Player()
{}

void Player::Tick( Map& map, const Time current_time, const Time last_tick_delta )
{
	Move( last_tick_delta );

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
			const float reloading_time= float( game_resources_->weapons_description[ current_weapon_index_ ].reloading_time ) / 100.0f;
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
		if( weapon_state_ == WeaponState::Idle && shoot_pressed_ )
		{
			const m_Vec3 view_vec( 0.0f, 1.0f, 0.0f );

			m_Mat4 x_rotate, z_rotate;
			x_rotate.RotateX( view_angle_x_ );
			z_rotate.RotateZ( view_angle_z_ );

			const m_Vec3 view_vec_rotated= view_vec * x_rotate * z_rotate;

			map.Shoot(
				game_resources_->weapons_description[ current_weapon_index_ ].r_type,
				pos_ + m_Vec3( 0.0f, 0.0f, GameConstants::player_eyes_level ),
				view_vec_rotated,
				current_time );

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

void Player::Hit( const int damage, const Time current_time )
{
	PC_UNUSED( current_time );

	// TODO - in original game damage is bigger or smaller, sometimes. Know, why.

	// Armor absorbess all damage and gives 1/4 of absorbed damage to health.
	int armor_damage= damage;
	if( armor_damage > armor_ )
		armor_damage= armor_;

	int health_damage= ( damage - armor_damage ) + armor_damage / 4;

	armor_-= armor_damage;
	health_-= health_damage;
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

bool Player::TryActivateProcedure( const unsigned int proc_number, const Time current_time )
{
	PC_ASSERT( proc_number != 0u );

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

bool Player::TryPickupItem( const unsigned int item_id )
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

		return true;
	}
	else if( a_code == ACode::Item_Life )
	{
		if( health_ < 100 )
		{
			health_= std::min( health_ + 20, 100 );
			return true;
		}
		return false;
	}
	else if( a_code == ACode::Item_BigLife )
	{
		if( health_ < 200 )
		{
			health_= std::min( health_ + 100, 200 );
			return true;
		}
		return false;
	}
	else if( a_code == ACode::Item_Armor )
	{
		if( armor_ < 200 )
		{
			armor_= std::min( armor_ + 200, 200 );
			return true;
		}
		return false;
	}
	else if( a_code == ACode::Item_Helmet )
	{
		if( armor_ < 200 )
		{
			armor_= std::min( armor_ + 100, 200 );
			return true;
		}
		return false;
	}

	return false;
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
}

void Player::BuildWeaponMessage( Messages::PlayerWeapon& out_weapon_message ) const
{
	out_weapon_message.current_weapon_index_= current_weapon_index_;
	out_weapon_message.animation= current_weapon_animation_;
	out_weapon_message.animation_frame= current_weapon_animation_frame_;

	out_weapon_message.switch_stage= static_cast<unsigned int>( weapon_switch_stage_ * 254.9f );
}

void Player::UpdateMovement( const Messages::PlayerMove& move_message )
{
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
}

void Player::Move( const Time time_delta )
{
	const float time_delta_s= time_delta.ToSeconds();

	// TODO - calibrate this
	const float c_max_speed= 5.0f;
	const float c_acceleration= 40.0f;
	const float c_deceleration= 20.0f;
	const float c_jump_speed_delta= 3.3f;

	const float speed_delta= time_delta_s * mevement_acceleration_ * c_acceleration;
	const float deceleration_speed_delta= time_delta_s * c_deceleration;

	// Accelerate
	speed_.x+= std::cos( movement_direction_ ) * speed_delta;
	speed_.y+= std::sin( movement_direction_ ) * speed_delta;

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

	// Clamp speed
	const float new_speed_square_length= speed_.xy().SquareLength();
	if( new_speed_square_length > c_max_speed * c_max_speed )
	{
		const float k= c_max_speed / std::sqrt( new_speed_square_length );
		speed_.x*= k;
		speed_.y*= k;
	}

	// Fall down
	speed_.z+= GameConstants::vertical_acceleration * time_delta_s;

	// Jump
	if( jump_pessed_ && noclip_ )
		speed_.z-= 2.0f * GameConstants::vertical_acceleration * time_delta_s;
	else if( jump_pessed_ && on_floor_ && speed_.z <= 0.0f )
		speed_.z+= c_jump_speed_delta;

	// Clamp vertical speed
	if( std::abs( speed_.z ) > GameConstants::max_vertical_speed )
		speed_.z*= GameConstants::max_vertical_speed / std::abs( speed_.z );

	pos_+= speed_ * time_delta_s;

	if( noclip_ && pos_.z < 0.0f )
	{
		pos_.z= 0.0f;
		speed_.z= 0.0f;
	}
}

void Player::SetNoclip( const bool noclip )
{
	noclip_= noclip;
}

bool Player::IsNoclip() const
{
	return noclip_;
}

void Player::GiveWeapon()
{
	for( unsigned int w= 0u; w < GameConstants::weapon_count; w++ )
	{
		have_weapon_[w]= true;
		ammo_[w]= game_resources_->weapons_description[w].limit;
	}
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

unsigned int Player::CurrentAnimation() const
{
	return current_weapon_animation_;
}

unsigned int Player::CurrentAnimationFrame() const
{
	return current_weapon_animation_frame_;
}

} // namespace PanzerChasm
