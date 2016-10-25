#include "assert.hpp"
#include "log.hpp"
#include "math_utils.hpp"
#include "messages_extractor.inl"

#include "client.hpp"

namespace PanzerChasm
{

Client::Client(
	const GameResourcesConstPtr& game_resources,
	const MapLoaderPtr& map_loader,
	const LoopbackBufferPtr& loopback_buffer,
	const RenderingContext& rendering_context )
	: game_resources_(game_resources)
	, map_loader_(map_loader)
	, loopback_buffer_(loopback_buffer)
	, camera_controller_(
		m_Vec3( 0.0f, 0.0f, 0.0f ),
		float(rendering_context.viewport_size.Width()) / float(rendering_context.viewport_size.Height()) )
	, map_drawer_( game_resources, rendering_context )
{
	PC_ASSERT( game_resources_ != nullptr );
	PC_ASSERT( map_loader_ != nullptr );

	connection_info_.reset( new ConnectionInfo( loopback_buffer_->GetClientSideConnection() ) );

	map_drawer_.SetMap( map_loader_->LoadMap( 1u ) );
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
	} // for events
}

void Client::Loop()
{
	if( connection_info_ != nullptr )
		connection_info_->messages_extractor.ProcessMessages( *this );

	camera_controller_.Tick();

	if( connection_info_ != nullptr )
	{
		float move_direction, move_acceleration;
		camera_controller_.GetAcceleration( move_direction, move_acceleration );

		Messages::PlayerMove message;
		message.message_id= MessageId::PlayerMove;
		message.acceleration= static_cast<unsigned char>( move_acceleration * 254.5f );
		message.angle= static_cast<unsigned short>( move_direction * 65536.0f / Constants::two_pi );

		connection_info_->messages_sender.SendUnreliableMessage( message );
		connection_info_->messages_sender.Flush();
	}

	//Log::Info( "Pos: ", player_position_.x, " ", player_position_.y, " ", player_position_.z );
}

void Client::Draw()
{
	m_Mat4 view_matrix;
	player_position_.z= 8.0f;
	camera_controller_.GetViewMatrix( player_position_, view_matrix );
	map_drawer_.Draw( view_matrix );
}

void Client::operator()( const Messages::MessageBase& message )
{
	PC_ASSERT(false);
	Log::Warning( "Unknown message for server: ", int(message.message_id) );
}

void Client::operator()( const Messages::PlayerPosition& message )
{
	for( unsigned int j= 0u; j < 3u; j++ )
		player_position_.ToArr()[j]= float(message.xyz[j]) / 256.0f;
}

} // namespace PanzerChasm
