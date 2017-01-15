#pragma once
#include <vector>

#include "server/i_connections_listener.hpp"

namespace PanzerChasm
{

class LoopbackBuffer final : public IConnectionsListener
{
public:
	LoopbackBuffer();
	virtual ~LoopbackBuffer() override;

	void RequestConnect();
	void RequestDisconnect();

	IConnectionPtr GetClientSideConnection();

public: // IConnectionsListener
	virtual IConnectionPtr GetNewConnection() override;

private:
	class Connection;

	class Queue final
	{
	public:
		Queue();
		~Queue();

		unsigned int Size() const;
		void Clear();

		void PushBytes( const void* data, unsigned int data_size );
		void PopBytes( void* out_data, unsigned int data_size );

	private:
		void TryShrink();

	private:
		std::vector<unsigned char> buffer_;
		unsigned int pos_;
	};

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

	Queue client_to_server_reliable_buffer_;
	Queue client_to_server_unreliable_buffer_;

	Queue server_to_client_reliable_buffer_;
	Queue server_to_client_unreliable_buffer_;
};

} // namespace PanzerChasm
