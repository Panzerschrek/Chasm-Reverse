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

	if( map_state_ != nullptr )
		map_state_->Tick( Time::CurrentTime() );

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
		pos.z+= 0.5f * 1.75f;
		camera_controller_.GetViewMatrix( pos, view_matrix );

		map_drawer_.Draw( *map_state_, view_matrix );
	}
}

void Client::operator()( const Messages::MessageBase& message )
{
	PC_ASSERT(false);
	Log::Warning( "Unknown message for server: ", int(message.message_id) );
}

void Client::operator()( const Messages::EntityState& message )
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
	for( unsigned int j= 0u; j < 3u; j++ )
		player_position_.ToArr()[j]= float(message.xyz[j]) / 256.0f;
}

void Client::operator()( const Messages::StaticModelState& message )
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

void Client::operator()( const Messages::EntityBirth& message )
{
	if( map_state_ != nullptr )
		map_state_->ProcessMessage( message );
}

void Client::operator()( const Messages::EntityDeath& message )
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
			const MapData::Message& text_message= current_map_data_->messages[ message.text_message_number ];
			for( const  MapData::Message::Text& text : text_message.texts )
				Log::Info( text.data );
		}
	}
}

} // namespace PanzerChasm
