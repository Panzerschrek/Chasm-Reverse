#pragma once

#include <vec.hpp>
#include <matrix.hpp>

#include "math_utils.hpp"
#include "time.hpp"

namespace PanzerChasm
{

class MovementController final
{
public:
	MovementController(
		const m_Vec3& angle= m_Vec3(0.0f, 0.0f, 0.0f),
		float aspect= 1.0f, float fov= Constants::half_pi );

	~MovementController();

	void Tick();

	void GetAcceleration( float& out_dir, float& out_acceleration ) const;

	void GetViewMatrix( const m_Vec3& pos, m_Mat4& out_mat ) const;

	m_Vec3 GetCamDir() const;

	void RotateX( int delta );
	void RotateY( int delta );

	void ForwardPressed();
	void BackwardPressed();
	void LeftPressed();
	void RightPressed();
	void ForwardReleased();
	void BackwardReleased();
	void LeftReleased();
	void RightReleased();

	void UpPressed();
	void DownPressed();
	void UpReleased();
	void DownReleased();

	void RotateUpPressed();
	void RotateDownPressed();
	void RotateLeftPressed();
	void RotateRightPressed();
	void RotateUpReleased();
	void RotateDownReleased();
	void RotateLeftReleased();
	void RotateRightReleased();

private:
	m_Vec3 angle_;

	float aspect_;
	float fov_;

	bool forward_pressed_, backward_pressed_, left_pressed_, right_pressed_;
	bool up_pressed_, down_pressed_;
	bool rotate_up_pressed_, rotate_down_pressed_, rotate_left_pressed_, rotate_right_pressed_;

	Time prev_calc_tick_;
};

} // namespace ChasmReverse
