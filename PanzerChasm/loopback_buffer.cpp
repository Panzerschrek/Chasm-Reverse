#include <cstring>

#include "assert.hpp"

#include "loopback_buffer.hpp"

namespace PanzerChasm
{

class LoopbackBuffer::Connection final : public IConnection
{
public:
	Connection(
		Buffer& in_reliable_buffer,
		Buffer& in_unreliable_buffer,
		Buffer& out_reliable_buffer,
		Buffer& out_unreliable_buffer );

	virtual ~Connection() override;

public: // IConnection
	virtual void SendReliablePacket( const void* data, unsigned int data_size ) override;
	virtual void SendUnreliablePacket( const void* data, unsigned int data_size ) override;

	virtual unsigned int ReadRealiableData( void* out_data, unsigned int buffer_size ) override;
	virtual unsigned int ReadUnrealiableData( void* out_data, unsigned int buffer_size ) override;

	virtual void Disconnect() override;

private:
	Buffer& in_reliable_buffer_;
	Buffer& in_unreliable_buffer_;
	Buffer& out_reliable_buffer_;
	Buffer& out_unreliable_buffer_;
};

LoopbackBuffer::Connection::Connection(
	Buffer& in_reliable_buffer,
	Buffer& in_unreliable_buffer,
	Buffer& out_reliable_buffer,
	Buffer& out_unreliable_buffer )
	: in_reliable_buffer_(in_reliable_buffer)
	, in_unreliable_buffer_(in_unreliable_buffer)
	, out_reliable_buffer_(out_reliable_buffer)
	, out_unreliable_buffer_(out_unreliable_buffer)
{}

LoopbackBuffer::Connection::~Connection()
{}

void LoopbackBuffer::Connection::SendReliablePacket( const void *data, unsigned int data_size )
{
	in_reliable_buffer_.insert(
		in_reliable_buffer_.end(),
		static_cast<const unsigned char*>(data),
		static_cast<const unsigned char*>(data) + data_size );
}

void LoopbackBuffer::Connection::SendUnreliablePacket( const void *data, unsigned int data_size )
{
	in_unreliable_buffer_.insert(
		in_unreliable_buffer_.end(),
		static_cast<const unsigned char*>(data),
		static_cast<const unsigned char*>(data) + data_size );
}

unsigned int LoopbackBuffer::Connection::ReadRealiableData( void* out_data, unsigned int buffer_size )
{
	unsigned int result_size= std::min( buffer_size, out_reliable_buffer_.size() );

	std::memcpy( out_data, &*out_reliable_buffer_.begin(), result_size );
	out_reliable_buffer_.erase( out_reliable_buffer_.begin(), out_reliable_buffer_.begin() + result_size );

	return result_size;
}

unsigned int LoopbackBuffer::Connection::ReadUnrealiableData( void* out_data, unsigned int buffer_size )
{
	unsigned int result_size= std::min( buffer_size, out_reliable_buffer_.size() );

	std::memcpy( out_data, &*out_unreliable_buffer_.begin(), result_size );
	out_unreliable_buffer_.erase( out_unreliable_buffer_.begin(), out_unreliable_buffer_.begin() + result_size );

	return result_size;
}

void LoopbackBuffer::Connection::Disconnect()
{
	// TODO
	PC_ASSERT(false);
}

LoopbackBuffer::LoopbackBuffer()
{
	client_side_connection_=
		std::make_shared<Connection>(
			server_to_client_reliable_buffer_,
			server_to_client_unreliable_buffer_,
			client_to_server_reliable_buffer_,
			client_to_server_unreliable_buffer_ );

	server_side_connection_=
		std::make_shared<Connection>(
			client_to_server_reliable_buffer_,
			client_to_server_unreliable_buffer_,
			server_to_client_reliable_buffer_,
			server_to_client_unreliable_buffer_ );
}

LoopbackBuffer::~LoopbackBuffer()
{}

IConnectionPtr LoopbackBuffer::GetNewConnection()
{
	if( server_side_connection_ )
	{
		return server_side_connection_;
	}

	return nullptr;
}

} // namespace PanzerChasm
