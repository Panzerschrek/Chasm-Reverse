#include "camera_controller.hpp"

namespace ChasmReverse
{

CameraController::CameraController(
	const m_Vec3& pos,
	const m_Vec3& angle,
	float aspect, float fov )
	: pos_(pos), angle_(angle), aspect_(aspect), fov_(fov)
	, forward_pressed_(false), backward_pressed_(false), left_pressed_(false), right_pressed_(false)
	, up_pressed_(false), down_pressed_(false)
	, rotate_up_pressed_(false), rotate_down_pressed_(false), rotate_left_pressed_(false), rotate_right_pressed_(false)
	, prev_calc_tick_(std::chrono::steady_clock::now())
{}

CameraController::~CameraController()
{}

void CameraController::Tick()
{
	const auto new_tick= std::chrono::steady_clock::now();

	const float dt_s=
		static_cast<float>( std::chrono::duration_cast<std::chrono::milliseconds>(new_tick - prev_calc_tick_).count() ) *
		0.001f;

	prev_calc_tick_= new_tick;

	m_Vec3 rotate_vec( 0.0f ,0.0f, 0.0f );
	if(rotate_up_pressed_) rotate_vec.x+= 1.0f;
	if(rotate_down_pressed_) rotate_vec.x+= -1.0f;
	if(rotate_left_pressed_) rotate_vec.z+= 1.0f;
	if(rotate_right_pressed_) rotate_vec.z+= -1.0f;

	const float rot_speed= 1.75f;
	angle_+= dt_s * rot_speed * rotate_vec;
	
	if( angle_.y > Constants::two_pi ) angle_.y-= Constants::two_pi;
	else if( angle_.y < 0.0f ) angle_.y+= Constants::two_pi;
	if( angle_.x > Constants::half_pi ) angle_.x= Constants::half_pi;
	else if( angle_.x < -Constants::half_pi ) angle_.x= -Constants::half_pi;

	m_Vec3 move_vector(0.0f,0.0f,0.0f);

	if(forward_pressed_) move_vector.y+= 1.0f;
	if(backward_pressed_) move_vector.y+= -1.0f;
	if(left_pressed_) move_vector.x+= -1.0f;
	if(right_pressed_) move_vector.x+= 1.0f;
	if(up_pressed_) move_vector.z+= 1.0f;
	if(down_pressed_) move_vector.z+= -1.0f;

	m_Mat4 move_vector_rot_mat;
	move_vector_rot_mat.RotateZ( angle_.z );

	move_vector= move_vector * move_vector_rot_mat;

	const float speed= 5.0f;
	pos_+= dt_s * speed * move_vector;
}

void CameraController::GetViewMatrix( m_Mat4& out_mat ) const
{
	m_Mat4 rot_x, rot_z, translate, perspective, basis_change;

	translate.Translate( -pos_ );
	rot_x.RotateX( -angle_.x );
	rot_z.RotateZ( -angle_.z );
	perspective.PerspectiveProjection( aspect_, fov_, 0.25f, 128.0f );

	basis_change.Identity();
	basis_change[5]= 0.0f;
	basis_change[6]= 1.0f;
	basis_change[9]= 1.0f;
	basis_change[10]= 0.0f;

	out_mat= translate * rot_z * rot_x * basis_change * perspective;
}

m_Vec3 CameraController::GetCamDir() const
{
	m_Mat4 rot_x, rot_y;
	rot_x.RotateX( angle_.x );
	rot_y.RotateY( angle_.y );

	return m_Vec3( 0.0f, 0.0f, 1.0f ) * rot_x * rot_y;
}

void CameraController::RotateX( int delta )
{
	angle_.x+= float(delta) * 0.01f;
}

void CameraController::RotateY( int delta )
{
	angle_.y+= float(delta) * 0.01f;
}

void CameraController::ForwardPressed()
{
	forward_pressed_= true;
}

void CameraController::BackwardPressed()
{
	backward_pressed_= true;
}

void CameraController::LeftPressed()
{
	left_pressed_= true;
}

void CameraController::RightPressed()
{
	right_pressed_= true;
}

void CameraController::ForwardReleased()
{
	forward_pressed_= false;
}

void CameraController::BackwardReleased()
{
	backward_pressed_= false;
}

void CameraController::LeftReleased()
{
	left_pressed_= false;
}

void CameraController::RightReleased()
{
	right_pressed_= false;
}

void CameraController::UpPressed()
{
	up_pressed_= true;
}

void CameraController::DownPressed()
{
	down_pressed_= true;
}

void CameraController::UpReleased()
{
	up_pressed_= false;
}

void CameraController::DownReleased()
{
	down_pressed_= false;
}

void CameraController::RotateUpPressed()
{
	rotate_up_pressed_= true;
}

void CameraController::RotateDownPressed()
{
	rotate_down_pressed_= true;
}

void CameraController::RotateLeftPressed()
{
	rotate_left_pressed_= true;
}

void CameraController::RotateRightPressed()
{
	rotate_right_pressed_= true;
}

void CameraController::RotateUpReleased()
{
	rotate_up_pressed_= false;
}

void CameraController::RotateDownReleased()
{
	rotate_down_pressed_= false;
}

void CameraController::RotateLeftReleased()
{
	rotate_left_pressed_= false;
}

void CameraController::RotateRightReleased()
{
	rotate_right_pressed_= false;
}

} // namespace ChasmReverse
