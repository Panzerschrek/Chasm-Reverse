#pragma once

#include <vec.hpp>

#include "../messages.hpp"
#include "../time.hpp"

namespace PanzerChasm
{

// Server side player logic here.
class Player final
{
public:
	Player();
	~Player();

	void SetPosition( const m_Vec3& pos );
	void ClampSpeed( const m_Vec3& clamp_surface_normal );
	void SetOnFloor( bool on_floor );

	void UpdateMovement( const Messages::PlayerMove& move_message );
	void Move( Time time_delta );

	void ResetNewPositionFlag();

	void GiveRedKey();
	void GiveGreenKey();
	void GiveBlueKey();

	bool HaveRedKey() const;
	bool HaveGreenKey() const;
	bool HaveBlueKey() const;

	m_Vec3 Position() const;
	unsigned char MapPositionX() const;
	unsigned char MapPositionY() const;
	bool MapPositionIsNew() const;

private:
	void UpdateMapPosition();

private:
	m_Vec3 pos_;
	m_Vec3 speed_;
	bool on_floor_;

	bool have_red_key_= false;
	bool have_green_key_= false;
	bool have_blue_key_= false;

	unsigned char map_position_[2];
	bool map_position_is_new_= true;

	float mevement_acceleration_= 0.0f;
	float movement_direction_= 0.0f;
	bool jump_pessed_= false;
};

} // namespace PanzerChasm
