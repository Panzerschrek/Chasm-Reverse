#include "../map_loader.hpp"
#include "./math_utils.hpp"

#include "player.hpp"

namespace PanzerChasm
{

Player::Player()
	: pos_( 0.0f, 0.0f, 0.0f )
	, speed_( 0.0f, 0.0f, 0.0f )
	, on_floor_(false)
	, noclip_(false)
{
}

Player::~Player()
{}

void Player::SetPosition( const m_Vec3& pos )
{
	pos_= pos;
}

void Player::ClampSpeed( const m_Vec3& clamp_surface_normal )
{
	const float projection= clamp_surface_normal * speed_;
	if( projection < 0.0f )
		speed_-= clamp_surface_normal * projection;
}

void Player::SetOnFloor( bool on_floor )
{
	on_floor_= on_floor;
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

void Player::UpdateMovement( const Messages::PlayerMove& move_message )
{
	mevement_acceleration_= float(move_message.acceleration) / 255.0f;
	movement_direction_= MessageAngleToAngle( move_message.angle );
	jump_pessed_= move_message.jump_pressed;
}

void Player::Move( const Time time_delta )
{
	const float time_delta_s= time_delta.ToSeconds();

	// TODO - calibrate this
	const float c_max_speed= 5.0f;
	const float c_acceleration= 40.0f;
	const float c_deceleration= 20.0f;
	const float c_jump_speed_delta= 3.3f;
	const float c_vertical_acceleration= -9.8f;
	const float c_max_vertical_speed= 5.0f;

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
	speed_.z+= c_vertical_acceleration * time_delta_s;

	// Jump
	if( jump_pessed_ && noclip_ )
		speed_.z-= 2.0f * c_vertical_acceleration * time_delta_s;
	else if( jump_pessed_ && on_floor_ && speed_.z <= 0.0f )
		speed_.z+= c_jump_speed_delta;

	// Clamp vertical speed
	if( std::abs( speed_.z ) > c_max_vertical_speed )
		speed_.z*= c_max_vertical_speed / std::abs( speed_.z );

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

m_Vec3 Player::Position() const
{
	return pos_;
}

} // namespace PanzerChasm
