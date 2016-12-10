#include <cstring>

#include "assert.hpp"
#include "i_connection.hpp"

#include "messages_sender.hpp"

namespace PanzerChasm
{

MessagesSender::MessagesSender( IConnectionPtr connection )
	: connection_( std::move(connection) )
{}

MessagesSender::~MessagesSender()
{}

void MessagesSender::Flush()
{
	if( unreliable_messages_buffer_pos_ > 0u )
	{
		connection_->SendUnreliablePacket( unreliable_messages_buffer_, unreliable_messages_buffer_pos_ );
		unreliable_messages_buffer_pos_= 0u;
	}
}

void MessagesSender::SendReliableMessageImpl( const void* const data, const unsigned int size )
{
	connection_->SendReliablePacket( data, size );
}

void MessagesSender::SendUnreliableMessageImpl( const void* const data, const unsigned int size )
{
	if( unreliable_messages_buffer_pos_ + size > sizeof(unreliable_messages_buffer_) )
	{
		Flush();
	}

	std::memcpy(
		unreliable_messages_buffer_ + unreliable_messages_buffer_pos_,
		data,
		size );

	unreliable_messages_buffer_pos_+= size;
}

} // namespace PanzerChasm
