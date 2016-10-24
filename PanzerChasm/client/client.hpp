#pragma once

#include "../connection_info.hpp"
#include "../game_resources.hpp"
#include "../loopback_buffer.hpp"
#include "../map_loader.hpp"
#include "../system_event.hpp"
#include "movement_controller.hpp"

namespace PanzerChasm
{

class Client final
{
public:
	Client(
		const GameResourcesConstPtr& game_resources,
		const MapLoaderPtr& map_loader,
		const LoopbackBufferPtr& loopback_buffer );

	~Client();

	void ProcessEvents( const SystemEvents& events );

	void Loop();

public: // Messages handlers
	void operator()( const Messages::MessageBase& message );
	void operator()( const Messages::PlayerPosition& message );

private:
	const GameResourcesConstPtr game_resources_;
	const MapLoaderPtr map_loader_;
	const LoopbackBufferPtr loopback_buffer_;

	std::unique_ptr<ConnectionInfo> connection_info_;

	m_Vec3 player_position_;
	MovementController camera_controller_;
};

} // PanzerChasm
