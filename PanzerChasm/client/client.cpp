#include "../assert.hpp"
#include "../game_constants.hpp"
#include "../log.hpp"
#include "../math_utils.hpp"
#include "../messages_extractor.inl"

#include "client.hpp"

namespace PanzerChasm
{

Client::Client(
	const GameResourcesConstPtr& game_resources,
	const MapLoaderPtr& map_loader,
	const LoopbackBufferPtr& loopback_buffer,
	const RenderingContext& rendering_context,
	const DrawersPtr& drawers )
	: game_resources_(game_resources)
	, map_loader_(map_loader)
	, loopback_buffer_(loopback_buffer)
	, current_tick_time_( Time::CurrentTime() )
	, camera_controller_(
		m_Vec3( 0.0f, 0.0f, 0.0f ),
		float(rendering_context.viewport_size.Width()) / float(rendering_context.viewport_size.Height()) )
	, map_drawer_( game_resources, rendering_context )
	, hud_drawer_( game_resources, rendering_context, drawers )
{
	PC_ASSERT( game_resources_ != nullptr );
	PC_ASSERT( map_loader_ != nullptr );

	connection_info_.reset( new ConnectionInfo( loopback_buffer_->GetClientSideConnection() ) );
}

Client::~Client()
{}

void Client::ProcessEvents( const SystemEvents& events )
{
	using KeyCode= SystemEvent::KeyEvent::KeyCode;

	for( const SystemEvent& event : events )
	{
		if( event.type == SystemEvent::Type::Key )
		{
			if( event.event.key.key_code == KeyCode::W )
				event.event.key.pressed ? camera_controller_.ForwardPressed() : camera_controller_.ForwardReleased();
			else if( event.event.key.key_code == KeyCode::S )
				event.event.key.pressed ? camera_controller_.BackwardPressed() : camera_controller_.BackwardReleased();
			else if( event.event.key.key_code == KeyCode::A )
				event.event.key.pressed ? camera_controller_.LeftPressed() : camera_controller_.LeftReleased();
			else if( event.event.key.key_code == KeyCode::D )
				event.event.key.pressed ? camera_controller_.RightPressed() : camera_controller_.RightReleased();
			else if( event.event.key.key_code == KeyCode::Space )
				event.event.key.pressed ? camera_controller_.UpPressed() : camera_controller_.UpReleased();
			else if( event.event.key.key_code == KeyCode::C )
				event.event.key.pressed ? camera_controller_.DownPressed() : camera_controller_.DownReleased();

			else if( event.event.key.key_code == KeyCode::Up )
				event.event.key.pressed ? camera_controller_.RotateUpPressed() : camera_controller_.RotateUpReleased();
			else if( event.event.key.key_code == KeyCode::Down )
				event.event.key.pressed ? camera_controller_.RotateDownPressed() : camera_controller_.RotateDownReleased();
			else if( event.event.key.key_code == KeyCode::Left )
				event.event.key.pressed ? camera_controller_.RotateLeftPressed() : camera_controller_.RotateLeftReleased();
			else if( event.event.key.key_code == KeyCode::Right )
				event.event.key.pressed ? camera_controller_.RotateRightPressed() : camera_controller_.RotateRightReleased();
		}
		else if( event.type == SystemEvent::Type::MouseKey &&
				event.event.mouse_key.pressed )
		{
			if( connection_info_ != nullptr )
			{
				Messages::PlayerShot message;
				message.message_id= MessageId::PlayerShot;

				message.view_dir_angle_x=
					static_cast<unsigned short>( float(camera_controller_.GetViewAngleX()) / Constants::two_pi * 65536.0f );
				message.view_dir_angle_z=
					static_cast<unsigned short>( float(camera_controller_.GetViewAngleZ()) / Constants::two_pi * 65536.0f );

				connection_info_->messages_sender.SendUnreliableMessage( message );
			}
		}
	} // for events
}

void Client::Loop()
{
	current_tick_time_= Time::CurrentTime();

	if( connection_info_ != nullptr )
		connection_info_->messages_extractor.ProcessMessages( *this );

	camera_controller_.Tick();

	if( map_state_ != nullptr )
		map_state_->Tick( current_tick_time_ );

	if( connection_info_ != nullptr )
	{
		float move_direction, move_acceleration;
		camera_controller_.GetAcceleration( move_direction, move_acceleration );

		Messages::PlayerMove message;
		message.message_id= MessageId::PlayerMove;
		message.acceleration= static_cast<unsigned char>( move_acceleration * 254.5f );
		message.angle= static_cast<unsigned short>( move_direction * 65536.0f / Constants::two_pi );
		message.jump_pressed= camera_controller_.JumpPressed();

		connection_info_->messages_sender.SendUnreliableMessage( message );
		connection_info_->messages_sender.Flush();
	}

	//Log::Info( "Pos: ", player_position_.x, " ", player_position_.y, " ", player_position_.z );
}

void Client::Draw()
{
	if( map_state_ != nullptr )
	{
		m_Mat4 view_matrix;
		m_Vec3 pos= player_position_;
		pos.z+= GameConstants::player_eyes_level;
		camera_controller_.GetViewMatrix( pos, view_matrix );

		map_drawer_.Draw( *map_state_, view_matrix, pos );
		hud_drawer_.DrawCrosshair(2u);
		hud_drawer_.DrawCurrentMessage( 2u, current_tick_time_ );
		hud_drawer_.DrawHud( 2u );
	}
}

void Client::operator()( const Messages::MessageBase& message )
{
	PC_ASSERT(false);
	Log::Warning( "Unknown message for server: ", int(message.message_id) );
}

void Client::operator()( const Messages::MonsterState& message )
{
	if( map_state_ != nullptr )
		map_state_->ProcessMessage( message );
}

void Client::operator()( const Messages::WallPosition& message )
{
	if( map_state_ != nullptr )
		map_state_->ProcessMessage( message );
}

void Client::operator()( const Messages::PlayerPosition& message )
{
	MessagePositionToPosition( message.xyz, player_position_ );
}

void Client::operator()( const Messages::StaticModelState& message )
{
	if( map_state_ != nullptr )
		map_state_->ProcessMessage( message );
}

void Client::operator()( const Messages::SpriteEffectBirth& message )
{
	if( map_state_ != nullptr )
		map_state_->ProcessMessage( message );
}

void Client::operator()( const Messages::MapChange& message )
{
	const MapDataConstPtr map_data= map_loader_->LoadMap( message.map_number );
	if( map_data == nullptr )
	{
		// TODO - handel error
		PC_ASSERT(false);
		return;
	}

	map_drawer_.SetMap( map_data );
	map_state_.reset( new MapState( map_data, game_resources_, Time::CurrentTime() ) );

	current_map_data_= map_data;
}

void Client::operator()( const Messages::MonsterBirth& message )
{
	if( map_state_ != nullptr )
		map_state_->ProcessMessage( message );
}

void Client::operator()( const Messages::MonsterDeath& message )
{
	if( map_state_ != nullptr )
		map_state_->ProcessMessage( message );
}

void Client::operator()( const Messages::TextMessage& message )
{
	if( current_map_data_ != nullptr )
	{
		if( message.text_message_number < current_map_data_->messages.size() )
		{
			hud_drawer_.AddMessage( current_map_data_->messages[ message.text_message_number ], current_tick_time_ );
		}
	}
}

void Client::operator()( const Messages::RocketState& message )
{
	if( map_state_ != nullptr )
		map_state_->ProcessMessage( message );
}

void Client::operator()( const Messages::RocketBirth& message )
{
	if( map_state_ != nullptr )
		map_state_->ProcessMessage( message );
}

void Client::operator()( const Messages::RocketDeath& message )
{
	if( map_state_ != nullptr )
		map_state_->ProcessMessage( message );
}

} // namespace PanzerChasm
