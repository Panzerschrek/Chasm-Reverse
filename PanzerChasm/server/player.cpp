#include "../map_loader.hpp"
#include "./math_utils.hpp"

#include "player.hpp"

namespace PanzerChasm
{

static const unsigned char g_no_map_position= 255u;

Player::Player()
	: pos_( 0.0f, 0.0f, 0.0f )
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

void Player::UpdateMovement( const Messages::PlayerMove& move_message )
{
	mevement_acceleration_= float(move_message.acceleration) / 255.0f;
	movement_direction_= MessageAngleToAngle( move_message.angle );
	jump_pessed= move_message.jump_pressed;
}

void Player::Move( const Time time_delta )
{
	const float c_max_speed= 5.0f;
	const float speed= c_max_speed * mevement_acceleration_;

	const float delta= time_delta.ToSeconds() * speed;

	pos_.x+= delta * std::cos( movement_direction_ );
	pos_.y+= delta * std::sin( movement_direction_ );

	if( jump_pessed )
		pos_.z+= +1.5f * time_delta.ToSeconds();
	else
		pos_.z+= -0.5f * time_delta.ToSeconds();

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
