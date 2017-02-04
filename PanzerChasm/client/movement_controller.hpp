#pragma once

#include <vec.hpp>
#include <matrix.hpp>

#include "fwd.hpp"
#include "math_utils.hpp"
#include  "system_event.hpp"
#include "time.hpp"

namespace PanzerChasm
{

class MovementController final
{
public:
	MovementController(
		const Settings& settings,
		const m_Vec3& angle= m_Vec3(0.0f, 0.0f, 0.0f),
		float aspect= 1.0f, float fov= Constants::half_pi );

	~MovementController();

	void Tick( const KeyboardState& keyboard_state );
	void SetSpeed( float speed );
	void SetAngles( float z_angle, float x_angle );

	void GetAcceleration(
		const KeyboardState& keyboard_state,
		float& out_dir, float& out_acceleration ) const;

	float GetEyeZShift() const;

	void GetViewProjectionMatrix(  m_Mat4& out_mat ) const;
	void GetViewRotationAndProjectionMatrix( m_Mat4& out_mat ) const;
	void GetViewMatrix( const m_Vec3& pos, m_Mat4& out_mat ) const;

	m_Vec3 GetCamDir() const;
	float GetViewAngleX() const;
	float GetViewAngleZ() const;

	bool JumpPressed() const;

	void RotateX( int delta );
	void RotateZ( int delta );

private:
	const Settings& settings_;

	m_Vec3 angle_;

	float aspect_;
	float fov_;

	float speed_;

	bool jump_pressed_;

	const Time start_tick_;
	Time prev_calc_tick_;
};

} // namespace ChasmReverse
