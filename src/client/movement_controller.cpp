#include <algorithm>

#include "game_constants.hpp"
#include "settings.hpp"
#include "shared_settings_keys.hpp"
#include "movement_controller.hpp"
#include "../key_checker.hpp"

namespace PanzerChasm
{

static const float g_z_near= 1.0f / 12.0f; // Must be greater, then z_near in software rasterizer.

MovementController::MovementController(
	Settings& settings,
	const m_Vec3& angle,
	float aspect )
	: settings_( settings )
	, angle_(angle), aspect_(aspect)
	, speed_(0.0f)
	, mouse_look_( settings_.GetOrSetBool( SettingsKeys::perm_mlook, true ) )
	, mouse_look_pressed_( false )
	, start_tick_( Time::CurrentTime() )
	, prev_calc_tick_( Time::CurrentTime() )
{
	UpdateParams();
}

MovementController::~MovementController()
{}

void MovementController::SetAspect( const float aspect )
{
	aspect_= aspect;
}

void MovementController::UpdateParams()
{
	fov_= settings_.GetOrSetFloat( SettingsKeys::fov, 90.0f );
	fov_= std::max( 10.0f, std::min( fov_, 150.0f ) );
	settings_.SetSetting( SettingsKeys::fov, fov_ );

	is_old_style_perspective_= settings_.GetOrSetBool( SettingsKeys::old_style_perspective, false );

	ClipCameraAngles();
}

void MovementController::Tick( const InputState& input_state )
{
	const Time new_tick= Time::CurrentTime();

	const float dt_s= ( new_tick - prev_calc_tick_ ).ToSeconds();

	prev_calc_tick_= new_tick;

	const KeyChecker key_pressed( settings_, input_state );

	m_Vec3 rotate_vec( 0.0f ,0.0f, 0.0f );

	if( key_pressed( SettingsKeys::key_perm_mlook, KeyCode::E ) ) mouse_look_ = !mouse_look_;
	if( key_pressed( SettingsKeys::key_turn_left  , KeyCode::KP4 ) ) rotate_vec.z+= +1.0f;
	if( key_pressed( SettingsKeys::key_turn_right , KeyCode::KP6 ) ) rotate_vec.z+= -1.0f;
	if( key_pressed( SettingsKeys::key_look_up    , KeyCode::KP8 ) ) rotate_vec.x+= +1.0f;
	if( key_pressed( SettingsKeys::key_look_down  , KeyCode::KP2 ) ) rotate_vec.x+= -1.0f;
	if( key_pressed( SettingsKeys::key_center_view, KeyCode::KP5 ) ) angle_.x = 0.0f;

	const float rot_speed= 1.75f;
	angle_+= dt_s * rot_speed * rotate_vec;

	ClipCameraAngles();

	jump_pressed_= key_pressed( SettingsKeys::key_jump, KeyCode::Mouse2 );
	mouse_look_pressed_ = key_pressed( SettingsKeys::key_temp_mlook, KeyCode::Q );
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
	const InputState& input_state,
	float& out_dir, float& out_acceleration ) const
{
	m_Vec3 move_vector(0.0f,0.0f,0.0f);

	const KeyChecker key_pressed( settings_,input_state );

	if( key_pressed( SettingsKeys::key_forward   , KeyCode::W ) ) move_vector.y+= +1.0f;
	if( key_pressed( SettingsKeys::key_backward  , KeyCode::S ) ) move_vector.y+= -1.0f;
	if( key_pressed( SettingsKeys::key_step_left , KeyCode::A ) ) move_vector.x+= -1.0f;
	if( key_pressed( SettingsKeys::key_step_right, KeyCode::D ) ) move_vector.x+= +1.0f;

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
	const float c_z_far= 128.0f;

	m_Mat4 perspective, basis_change;

	basis_change.Identity();
	basis_change[5]= 0.0f;
	basis_change[6]= 1.0f;
	basis_change[9]= 1.0f;
	basis_change[10]= 0.0f;

	perspective.PerspectiveProjection( aspect_, fov_ * Constants::to_rad, g_z_near, c_z_far );

	out_mat= basis_change * perspective;
}

void MovementController::GetViewRotationAndProjectionMatrix( m_Mat4& out_mat ) const
{
	m_Mat4 rot_x, rot_z, projection;

	rot_z.RotateZ( -angle_.z );
	GetViewProjectionMatrix( projection );

	if( is_old_style_perspective_ )
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

void MovementController::GetViewClipPlanes( const m_Vec3& pos, ViewClipPlanes& out_clip_planes ) const
{
	// Extend view field to prevent clipping errors.
	const float c_fov_scaler= 1.05f;

	const float half_fov_x= std::atan( aspect_ * std::tan( fov_ * ( Constants::to_rad * 0.5f ) ) ) * c_fov_scaler;
	const float half_fov_y= fov_ * ( c_fov_scaler * Constants::to_rad * 0.5f );

	m_Mat4 rot_x, rot_z, rot;
	rot_x.RotateX( angle_.x );
	rot_z.RotateZ( angle_.z );
	if( is_old_style_perspective_ )
		rot= rot_z;
	else
		rot= rot_x * rot_z;

	// Near clip plane
	out_clip_planes[0].normal= m_Vec3( 0.0f, 1.0f, 0.0f ) * rot;
	out_clip_planes[0].dist= -( pos * out_clip_planes[0].normal + g_z_near );
	// Left
	out_clip_planes[1].normal= m_Vec3( +std::cos( half_fov_x ), +std::sin( half_fov_x ), 0.0f ) * rot;
	out_clip_planes[1].dist= -( pos * out_clip_planes[1].normal );
	// Right
	out_clip_planes[2].normal= m_Vec3( -std::cos( half_fov_x ), +std::sin( half_fov_x ), 0.0f ) * rot;
	out_clip_planes[2].dist= -( pos * out_clip_planes[2].normal );

	if( is_old_style_perspective_ )
	{
		const float shift= std::cos( half_fov_y ) * std::tan( angle_.x );

		// Bottom
		out_clip_planes[3].normal= m_Vec3( 0.0f, +std::sin( half_fov_y ) - shift, +std::cos( half_fov_y ) );
		out_clip_planes[3].normal.Normalize();
		out_clip_planes[3].normal= out_clip_planes[3].normal * rot_z;
		// Top
		out_clip_planes[4].normal= m_Vec3( 0.0f, +std::sin( half_fov_y ) + shift, -std::cos( half_fov_y ) );
		out_clip_planes[4].normal.Normalize();
		out_clip_planes[4].normal= out_clip_planes[4].normal * rot_z;
	}
	else
	{
		// Bottom
		out_clip_planes[3].normal= m_Vec3( 0.0f, +std::sin( half_fov_y ), +std::cos( half_fov_y ) ) * rot;
		// Top
		out_clip_planes[4].normal= m_Vec3( 0.0f, +std::sin( half_fov_y ), -std::cos( half_fov_y ) ) * rot;

	}
	out_clip_planes[3].dist= -( pos * out_clip_planes[3].normal );
	out_clip_planes[4].dist= -( pos * out_clip_planes[4].normal );
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
	if( ( mouse_look_pressed_ && !mouse_look_ ) || ( !mouse_look_pressed_ && mouse_look_) )
	{
		float base_sensetivity= settings_.GetOrSetFloat( SettingsKeys::mouse_sensetivity, 0.5f );
		base_sensetivity= std::max( 0.0f, std::min( base_sensetivity, 1.0f ) );
		settings_.SetSetting( SettingsKeys::mouse_sensetivity, base_sensetivity );

		float d_x_f, d_z_f;
		if( settings_.GetOrSetBool( "cl_mouse_filter", true ) )
		{
			d_x_f= float( delta_x + prev_controller_delta_x_ ) * 0.5f;
			d_z_f= float( delta_z + prev_controller_delta_z_ ) * 0.5f;
		}
		else
		{
			d_x_f= float(delta_x);
			d_z_f= float(delta_z);
		}

		if( settings_.GetOrSetBool( "cl_mouse_acceleration", true ) )
		{
			const float c_acceleration= 0.25f;
			const float c_max_acceleration_factor= 2.0f;
			d_x_f= std::min( c_acceleration * std::sqrt(std::abs(d_x_f)), c_max_acceleration_factor ) * d_x_f;
			d_z_f= std::min( c_acceleration * std::sqrt(std::abs(d_z_f)), c_max_acceleration_factor ) * d_z_f;
		}

		const float c_max_exp_sensetivity= 8.0f;
		const float exp_sensetivity= std::exp( base_sensetivity * std::log(c_max_exp_sensetivity) ); // [ 1; c_max_exp_sensetivity ]

		const float c_pix_scale= 1.0f / 1024.0f;
		const float z_direction= settings_.GetOrSetBool( SettingsKeys::reverse_mouse ) ? -1.0f : +1.0f;

		angle_.x-= exp_sensetivity * c_pix_scale * d_x_f * z_direction;
		angle_.z-= exp_sensetivity * c_pix_scale * d_z_f * 0.5f;

		prev_controller_delta_x_= delta_x;
		prev_controller_delta_z_= delta_z;
	}
}

void MovementController::ClipCameraAngles()
{
	const float max_ange_x=
		is_old_style_perspective_
			? ( Constants::half_pi * 0.65f )
			: Constants::half_pi;

	if( angle_.z > Constants::two_pi ) angle_.z-= Constants::two_pi;
	else if( angle_.z < 0.0f ) angle_.z+= Constants::two_pi;
	if( angle_.x > max_ange_x ) angle_.x= max_ange_x;
	else if( angle_.x < -max_ange_x ) angle_.x= -max_ange_x;
}

} // namespace ChasmReverse
