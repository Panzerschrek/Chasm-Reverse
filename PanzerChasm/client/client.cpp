#include "assert.hpp"

#include "client.hpp"

namespace PanzerChasm
{

Client::Client(
	const GameResourcesConstPtr& game_resources,
	const MapLoaderPtr& map_loader,
	const LoopbackBufferPtr& loopback_buffer )
	: game_resources_(game_resources)
	, map_loader_(map_loader)
	, loopback_buffer_(loopback_buffer)
{
	PC_ASSERT( game_resources_ != nullptr );
	PC_ASSERT( map_loader_ != nullptr );
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
		}
	} // for events
}

void Client::Loop()
{
	camera_controller_.Tick();

	float move_direction, move_acceleration;
	camera_controller_.GetAcceleration( move_direction, move_acceleration );

}

} // namespace PanzerChasm
