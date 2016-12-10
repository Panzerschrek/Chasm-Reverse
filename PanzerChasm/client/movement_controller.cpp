#include "movement_controller.hpp"

namespace PanzerChasm
{

MovementController::MovementController(
	const m_Vec3& angle,
	float aspect, float fov )
	: angle_(angle), aspect_(aspect), fov_(fov)
	, forward_pressed_(false), backward_pressed_(false), left_pressed_(false), right_pressed_(false)
	, up_pressed_(false), down_pressed_(false)
	, rotate_up_pressed_(false), rotate_down_pressed_(false), rotate_left_pressed_(false), rotate_right_pressed_(false)
	, prev_calc_tick_( Time::CurrentTime() )
{}

MovementController::~MovementController()
{}

void MovementController::Tick()
{
	const Time new_tick= Time::CurrentTime();

	const float dt_s= ( new_tick - prev_calc_tick_ ).ToSeconds();

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
}

void MovementController::GetAcceleration( float& out_dir, float& out_acceleration ) const
{
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

	if( move_vector.xy().SquareLength() <= 0.001f )
	{
		out_dir= 0.0f;
		out_acceleration= 0.0f;
	}
	else
	{
		out_dir= std::atan2( move_vector.y, move_vector.x );
		out_acceleration= 1.0f;
	}
}

void MovementController::GetViewMatrix( const m_Vec3& pos, m_Mat4& out_mat ) const
{
	const bool old_style= false;

	m_Mat4 rot_x, rot_z, translate, perspective, basis_change;

	translate.Translate( -pos );
	rot_z.RotateZ( -angle_.z );
	perspective.PerspectiveProjection( aspect_, fov_, 0.125f, 128.0f );

	basis_change.Identity();
	basis_change[5]= 0.0f;
	basis_change[6]= 1.0f;
	basis_change[9]= 1.0f;
	basis_change[10]= 0.0f;

	if( old_style )
	{
		rot_x.Identity();
		rot_x.value[9]= std::tan( -angle_.x );

		out_mat= translate * rot_z * basis_change * rot_x * perspective;
	}
	else
	{
		rot_x.RotateX( -angle_.x );
		out_mat= translate * rot_z * rot_x * basis_change * perspective;
	}
}

m_Vec3 MovementController::GetCamDir() const
{
	m_Mat4 rot_x, rot_y;
	rot_x.RotateX( angle_.x );
	rot_y.RotateY( angle_.y );

	return m_Vec3( 0.0f, 0.0f, 1.0f ) * rot_x * rot_y;
}

float MovementController::GetViewAngleX() const
{
	return angle_.x;
}

float MovementController::GetViewAngleZ() const
{
	return angle_.z;
}

bool MovementController::JumpPressed() const
{
	return up_pressed_;
}

void MovementController::RotateX( int delta )
{
	angle_.x+= float(delta) * 0.004f;
}

void MovementController::RotateZ( int delta )
{
	angle_.z+= float(delta) * 0.004f;
}

void MovementController::ForwardPressed()
{
	forward_pressed_= true;
}

void MovementController::BackwardPressed()
{
	backward_pressed_= true;
}

void MovementController::LeftPressed()
{
	left_pressed_= true;
}

void MovementController::RightPressed()
{
	right_pressed_= true;
}

void MovementController::ForwardReleased()
{
	forward_pressed_= false;
}

void MovementController::BackwardReleased()
{
	backward_pressed_= false;
}

void MovementController::LeftReleased()
{
	left_pressed_= false;
}

void MovementController::RightReleased()
{
	right_pressed_= false;
}

void MovementController::UpPressed()
{
	up_pressed_= true;
}

void MovementController::DownPressed()
{
	down_pressed_= true;
}

void MovementController::UpReleased()
{
	up_pressed_= false;
}

void MovementController::DownReleased()
{
	down_pressed_= false;
}

void MovementController::RotateUpPressed()
{
	rotate_up_pressed_= true;
}

void MovementController::RotateDownPressed()
{
	rotate_down_pressed_= true;
}

void MovementController::RotateLeftPressed()
{
	rotate_left_pressed_= true;
}

void MovementController::RotateRightPressed()
{
	rotate_right_pressed_= true;
}

void MovementController::RotateUpReleased()
{
	rotate_up_pressed_= false;
}

void MovementController::RotateDownReleased()
{
	rotate_down_pressed_= false;
}

void MovementController::RotateLeftReleased()
{
	rotate_left_pressed_= false;
}

void MovementController::RotateRightReleased()
{
	rotate_right_pressed_= false;
}

} // namespace ChasmReverse
