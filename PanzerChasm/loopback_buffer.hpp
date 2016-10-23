#pragma once
#include <deque>

#include "server/i_connections_listener.hpp"

namespace PanzerChasm
{

class LoopbackBuffer final : public IConnectionsListener
{
public:
	LoopbackBuffer();
	virtual ~LoopbackBuffer() override;

	void RequestConnect();

public: // IConnectionsListener
	virtual IConnectionPtr GetNewConnection() override;

	class Connection;

	// Input - push_back
	// Output - pop_front
	typedef std::deque<unsigned char> Buffer;

	enum class State
	{
		Unconnected,
		WaitingForConnection,
		Connected,
	};

private:
	State state_= State::Unconnected;

	IConnectionPtr client_side_connection_;
	IConnectionPtr server_side_connection_;

	Buffer client_to_server_reliable_buffer_;
	Buffer client_to_server_unreliable_buffer_;

	Buffer server_to_client_reliable_buffer_;
	Buffer server_to_client_unreliable_buffer_;
};

} // namespace PanzerChasm
