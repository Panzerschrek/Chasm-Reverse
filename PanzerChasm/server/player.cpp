#include "../map_loader.hpp"
#include "./math_utils.hpp"

#include "player.hpp"

namespace PanzerChasm
{

static const unsigned char g_no_map_position= 255u;

Player::Player()
	: pos_( 0.0f, 0.0f, 0.0f )
	, speed_( 0.0f, 0.0f, 0.0f )
	, on_floor_(false)
	, map_position_{ g_no_map_position, g_no_map_position }
{
	UpdateMapPosition();
}

Player::~Player()
{}

void Player::SetPosition( const m_Vec3& pos )
{
	pos_= pos;
	UpdateMapPosition();
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
	if( jump_pessed_ && on_floor_ && speed_.z <= 0.0f )
		speed_.z+= c_jump_speed_delta;

	// Clamp vertical speed
	if( std::abs( speed_.z ) > c_max_vertical_speed )
		speed_.z*= c_max_vertical_speed / std::abs( speed_.z );

	pos_+= speed_ * time_delta_s;

	UpdateMapPosition();
}

void Player::ResetNewPositionFlag()
{
	map_position_is_new_= false;
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

unsigned char Player::MapPositionX() const
{
	return map_position_[0];
}

unsigned char Player::MapPositionY() const
{
	return map_position_[1];
}

bool Player::MapPositionIsNew() const
{
	return map_position_is_new_;
}

void Player::UpdateMapPosition()
{
	const int new_map_x= static_cast<int>( pos_.x );
	const int new_map_y= static_cast<int>( pos_.y );

	if( new_map_x != map_position_[0] ||
		new_map_y != map_position_[1] )
	{
		if( new_map_x < 0 || new_map_x >= int(MapData::c_map_size) ||
			new_map_y < 0 || new_map_y >= int(MapData::c_map_size) )
		{
			// Position outside map
			map_position_[0]= map_position_[1]= g_no_map_position;
		}
		else
		{
			map_position_[0]= new_map_x;
			map_position_[1]= new_map_y;
		}

		map_position_is_new_= true;
	}
}

} // namespace PanzerChasm
