#include <cstring>

#include "assert.hpp"
#include "i_connection.hpp"

#include "loopback_buffer.hpp"

namespace PanzerChasm
{

class LoopbackBuffer::Connection final : public IConnection
{
public:
	Connection(
		Queue& in_reliable_buffer,
		Queue& in_unreliable_buffer,
		Queue& out_reliable_buffer,
		Queue& out_unreliable_buffer );

	virtual ~Connection() override;

public: // IConnection
	virtual void SendReliablePacket( const void* data, unsigned int data_size ) override;
	virtual void SendUnreliablePacket( const void* data, unsigned int data_size ) override;

	virtual unsigned int ReadRealiableData( void* out_data, unsigned int buffer_size ) override;
	virtual unsigned int ReadUnrealiableData( void* out_data, unsigned int buffer_size ) override;

	virtual void Disconnect() override;
	virtual bool Disconnected() override;

private:
	Queue& in_reliable_buffer_;
	Queue& in_unreliable_buffer_;
	Queue& out_reliable_buffer_;
	Queue& out_unreliable_buffer_;

	bool disconnected_= false;
};

LoopbackBuffer::Connection::Connection(
	Queue& in_reliable_buffer,
	Queue& in_unreliable_buffer,
	Queue& out_reliable_buffer,
	Queue& out_unreliable_buffer )
	: in_reliable_buffer_(in_reliable_buffer)
	, in_unreliable_buffer_(in_unreliable_buffer)
	, out_reliable_buffer_(out_reliable_buffer)
	, out_unreliable_buffer_(out_unreliable_buffer)
{}

LoopbackBuffer::Connection::~Connection()
{}

void LoopbackBuffer::Connection::SendReliablePacket( const void *data, unsigned int data_size )
{
	if( disconnected_ ) return;
	in_reliable_buffer_.PushBytes( data, data_size );
}

void LoopbackBuffer::Connection::SendUnreliablePacket( const void *data, unsigned int data_size )
{
	if( disconnected_ ) return;
	in_unreliable_buffer_.PushBytes( data, data_size );
}

unsigned int LoopbackBuffer::Connection::ReadRealiableData( void* out_data, unsigned int buffer_size )
{
	if( disconnected_ ) return 0u;

	unsigned int result_size= std::min( buffer_size, out_reliable_buffer_.Size() );
	out_reliable_buffer_.PopBytes( out_data, result_size );
	return result_size;
}

unsigned int LoopbackBuffer::Connection::ReadUnrealiableData( void* out_data, unsigned int buffer_size )
{
	if( disconnected_ ) return 0u;

	unsigned int result_size= std::min( buffer_size, out_unreliable_buffer_.Size() );
	out_unreliable_buffer_.PopBytes( out_data, result_size );
	return result_size;
}

void LoopbackBuffer::Connection::Disconnect()
{
	disconnected_= true;
}

bool LoopbackBuffer::Connection::Disconnected()
{
	return disconnected_;
}

LoopbackBuffer::Queue::Queue()
	: pos_(0u)
{}

LoopbackBuffer::Queue::~Queue()
{}

unsigned int LoopbackBuffer::Queue::Size() const
{
	return buffer_.size() - pos_;
}

void LoopbackBuffer::Queue::Clear()
{
	buffer_.clear();
	pos_= 0u;
}

void LoopbackBuffer::Queue::PushBytes( const void* data, const unsigned int data_size )
{
	buffer_.insert(
		buffer_.end(),
		static_cast<const unsigned char*>(data),
		static_cast<const unsigned char*>(data) + data_size );
}

void LoopbackBuffer::Queue::PopBytes( void* out_data, const unsigned int data_size )
{
	PC_ASSERT( data_size <= Size() );

	std::memcpy(
		out_data,
		buffer_.data() + pos_,
		data_size );

	pos_+= data_size;

	TryShrink();
}

void LoopbackBuffer::Queue::TryShrink()
{
	// Some magic constants here.
	const unsigned int c_min_buffer_size_to_shrink= 64u;
	const unsigned int c_rate_mult= 16u;

	if( buffer_.size() < c_min_buffer_size_to_shrink )
		return;

	if( Size() * c_rate_mult < buffer_.size() )
	{
		buffer_.erase( buffer_.begin(), buffer_.begin() + pos_ );
		pos_= 0u;
	}
}

LoopbackBuffer::LoopbackBuffer()
{
}

LoopbackBuffer::~LoopbackBuffer()
{
	RequestDisconnect();
}

void LoopbackBuffer::RequestConnect()
{
	PC_ASSERT( state_ == State::Unconnected );

	client_side_connection_=
		std::make_shared<Connection>(
			client_to_server_reliable_buffer_,
			client_to_server_unreliable_buffer_,
			server_to_client_reliable_buffer_,
			server_to_client_unreliable_buffer_ );

	server_side_connection_=
		std::make_shared<Connection>(
			server_to_client_reliable_buffer_,
			server_to_client_unreliable_buffer_,
			client_to_server_reliable_buffer_,
			client_to_server_unreliable_buffer_ );

	state_= State::WaitingForConnection;
}

void LoopbackBuffer::RequestDisconnect()
{
	if( client_side_connection_ != nullptr )
	{
		client_side_connection_->Disconnect();
		client_side_connection_.reset();
	}

	if( server_side_connection_ != nullptr )
	{
		server_side_connection_->Disconnect();
		server_side_connection_.reset();
	}

	client_to_server_reliable_buffer_.Clear();
	client_to_server_unreliable_buffer_.Clear();
	server_to_client_reliable_buffer_.Clear();
	server_to_client_unreliable_buffer_.Clear();

	state_ = State::Unconnected;
}

IConnectionPtr LoopbackBuffer::GetClientSideConnection()
{
	return client_side_connection_;
}

IConnectionPtr LoopbackBuffer::GetNewConnection()
{
	if( state_ == State::WaitingForConnection )
	{
		state_= State::Connected;
		return server_side_connection_;
	}

	return nullptr;
}

} // namespace PanzerChasm
