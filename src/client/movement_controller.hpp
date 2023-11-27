#pragma once

#include <vec.hpp>
#include <matrix.hpp>

#include "../fwd.hpp"
#include "i_map_drawer.hpp"
#include "math_utils.hpp"
#include "system_event.hpp"
#include "time.hpp"

namespace PanzerChasm
{

class MovementController final
{
public:
	MovementController(
		Settings& settings,
		const m_Vec3& angle= m_Vec3(0.0f, 0.0f, 0.0f),
		float aspect= 1.0f );

	~MovementController();

	void SetAspect( float aspect );

	void UpdateParams();
	void Tick( const InputState& input_state_ );
	void SetSpeed( float speed );
	void SetAngles( float z_angle, float x_angle );

	void GetAcceleration(
		const InputState& input_state,
		float& out_dir, float& out_acceleration ) const;

	float GetEyeZShift() const;

	void GetViewProjectionMatrix(  m_Mat4& out_mat ) const;
	void GetViewRotationAndProjectionMatrix( m_Mat4& out_mat ) const;
	void GetViewMatrix( const m_Vec3& pos, m_Mat4& out_mat ) const;

	void GetViewClipPlanes( const m_Vec3& pos, ViewClipPlanes& out_clip_planes ) const;

	m_Vec3 GetCamDir() const;
	float GetViewAngleX() const;
	float GetViewAngleZ() const;

	bool JumpPressed() const;

	void ControllerRotate( int delta_x, int delta_z );

private:
	void ClipCameraAngles();

private:
	Settings& settings_;

	m_Vec3 angle_;
	float aspect_;

	// Settings variables. Fetch it once per frame.
	float fov_; // Degrees
	bool is_old_style_perspective_;

	float speed_;

	bool jump_pressed_;
	bool mouse_look_pressed_;
	bool mouse_look_;
	const Time start_tick_;
	Time prev_calc_tick_;

	int prev_controller_delta_x_= 0, prev_controller_delta_z_= 0;
};

} // namespace ChasmReverse
