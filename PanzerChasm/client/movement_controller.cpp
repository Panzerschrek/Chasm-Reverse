#include <algorithm>

#include "game_constants.hpp"
#include "settings.hpp"
#include "shared_settings_keys.hpp"

#include "movement_controller.hpp"

namespace PanzerChasm
{

static const char g_old_style_perspective[]= "cl_old_style_perspective";
static const char g_fov[]= "cl_fov";

MovementController::MovementController(
	Settings& settings,
	const m_Vec3& angle,
	float aspect )
	: settings_( settings )
	, angle_(angle), aspect_(aspect)
	, speed_(0.0f)
	, start_tick_( Time::CurrentTime() )
	, prev_calc_tick_( Time::CurrentTime() )
{}

MovementController::~MovementController()
{}

void MovementController::Tick( const KeyboardState& keyboard_state )
{
	const Time new_tick= Time::CurrentTime();

	const float dt_s= ( new_tick - prev_calc_tick_ ).ToSeconds();

	prev_calc_tick_= new_tick;

	auto key_pressed=
	[&]( const char* const key_setting_name )
	{
		using KeyCode= SystemEvent::KeyEvent::KeyCode;
		const KeyCode key= static_cast<KeyCode>( settings_.GetInt( key_setting_name ) );
		if( key > KeyCode::Unknown && key < KeyCode::KeyCount )
			return keyboard_state[ static_cast<unsigned int>( key ) ];
		return false;
	};

	m_Vec3 rotate_vec( 0.0f ,0.0f, 0.0f );
	if( key_pressed( SettingsKeys::key_turn_left  ) ) rotate_vec.z+= +1.0f;
	if( key_pressed( SettingsKeys::key_turn_right ) ) rotate_vec.z+= -1.0f;
	if( key_pressed( SettingsKeys::key_look_up    ) ) rotate_vec.x+= +1.0f;
	if( key_pressed( SettingsKeys::key_look_down  ) ) rotate_vec.x+= -1.0f;

	const float rot_speed= 1.75f;
	angle_+= dt_s * rot_speed * rotate_vec;
	
	const float max_ange_x=
		settings_.GetOrSetBool( g_old_style_perspective, false )
			? ( Constants::half_pi * 0.65f )
			: Constants::half_pi;

	if( angle_.z > Constants::two_pi ) angle_.z-= Constants::two_pi;
	else if( angle_.z < 0.0f ) angle_.z+= Constants::two_pi;
	if( angle_.x > max_ange_x ) angle_.x= max_ange_x;
	else if( angle_.x < -max_ange_x ) angle_.x= -max_ange_x;

	jump_pressed_= key_pressed( SettingsKeys::key_jump );
}

void MovementController::SetSpeed( const float speed )
{
	speed_= speed;
}

void MovementController::SetAngles( float z_angle, float x_angle )
{
	angle_.z= z_angle;
	angle_.x= x_angle;
}

void MovementController::GetAcceleration(
	const KeyboardState& keyboard_state,
	float& out_dir, float& out_acceleration ) const
{
	m_Vec3 move_vector(0.0f,0.0f,0.0f);

	auto key_pressed=
	[&]( const char* const key_setting_name )
	{
		using KeyCode= SystemEvent::KeyEvent::KeyCode;
		const KeyCode key= static_cast<KeyCode>( settings_.GetInt( key_setting_name ) );
		if( key > KeyCode::Unknown && key < KeyCode::KeyCount )
			return keyboard_state[ static_cast<unsigned int>( key ) ];
		return false;
	};

	if( key_pressed( SettingsKeys::key_forward    ) ) move_vector.y+= +1.0f;
	if( key_pressed( SettingsKeys::key_backward   ) ) move_vector.y+= -1.0f;
	if( key_pressed( SettingsKeys::key_step_left  ) ) move_vector.x+= -1.0f;
	if( key_pressed( SettingsKeys::key_step_right ) ) move_vector.x+= +1.0f;

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

float MovementController::GetEyeZShift() const
{
	const float time_s= ( prev_calc_tick_ - start_tick_ ).ToSeconds();

	const float c_amplitude= 0.0625f;
	const float c_frequency= 1.75f * Constants::two_pi;

	const float amplitude= c_amplitude * std::min( speed_ / GameConstants::player_max_speed, 1.0f );
	const float phase= time_s * c_frequency;

	return amplitude * std::sin( phase );
}

void MovementController::GetViewProjectionMatrix( m_Mat4& out_mat ) const
{
	const float c_z_near= 1.0f / 16.0f;
	const float c_z_far= 128.0f;

	m_Mat4 perspective, basis_change;

	basis_change.Identity();
	basis_change[5]= 0.0f;
	basis_change[6]= 1.0f;
	basis_change[9]= 1.0f;
	basis_change[10]= 0.0f;

	float fov= settings_.GetOrSetFloat( g_fov, 90.0f );
	fov= std::max( 10.0f, std::min( fov, 150.0f ) );
	settings_.SetSetting( g_fov, fov );

	perspective.PerspectiveProjection( aspect_, fov * Constants::to_rad, c_z_near, c_z_far );

	out_mat= basis_change * perspective;
}

void MovementController::GetViewRotationAndProjectionMatrix( m_Mat4& out_mat ) const
{
	m_Mat4 rot_x, rot_z, projection;

	rot_z.RotateZ( -angle_.z );
	GetViewProjectionMatrix( projection );

	if( settings_.GetOrSetBool( g_old_style_perspective, false ) )
	{
		rot_x.Identity();
		rot_x.value[6]= std::tan( -angle_.x );
	}
	else
		rot_x.RotateX( -angle_.x );

	out_mat= rot_z * rot_x * projection;
}

void MovementController::GetViewMatrix( const m_Vec3& pos, m_Mat4& out_mat ) const
{
	m_Mat4 translate, rotatation_and_perspective;

	translate.Translate( -pos );
	GetViewRotationAndProjectionMatrix( rotatation_and_perspective );

	out_mat= translate * rotatation_and_perspective;
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
	return jump_pressed_;
}

void MovementController::ControllerRotate( const int delta_x, const int delta_z )
{
	float base_sensetivity= settings_.GetOrSetFloat( SettingsKeys::mouse_sensetivity, 0.5f );
	base_sensetivity= std::max( 0.0f, std::min( base_sensetivity, 1.0f ) );
	settings_.SetSetting( SettingsKeys::mouse_sensetivity, base_sensetivity );

	const float c_max_exp_sensetivity= 8.0f;
	const float exp_sensetivity= std::exp( base_sensetivity * std::log(c_max_exp_sensetivity) ); // [ 1; c_max_exp_sensetivity ]

	const float c_pix_scale= 1.0f / 1024.0f;

	const float z_direction= settings_.GetOrSetBool( SettingsKeys::reverse_mouse ) ? -1.0f : +1.0f;

	angle_.x-= exp_sensetivity * c_pix_scale * float(delta_x) * z_direction;
	angle_.z-= exp_sensetivity * c_pix_scale * float(delta_z);
}

} // namespace ChasmReverse
