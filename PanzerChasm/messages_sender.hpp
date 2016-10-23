#pragma once

#include "i_connection.hpp"

namespace PanzerChasm
{

class MessagesSender final
{
public:
	explicit MessagesSender( IConnectionPtr connection );
	~MessagesSender();

	template<class Message>
	void SendReliableMessage( const Message& message )
	{
		static_assert( std::is_pod<Message>::value, "Expected POD" );
		SendReliableMessageImpl( &message, sizeof(Message) );
	}

	template<class Message>
	void SendUnreliableMessage( const Message& message )
	{
		static_assert( std::is_pod<Message>::value, "Expected POD" );
		SendReliableMessageImpl( &message, sizeof(Message) );
	}

	void Flush();

private:
	void SendReliableMessageImpl( const void* data, unsigned int size );
	void SendUnreliableMessageImpl( const void* data, unsigned int size );

private:
	const IConnectionPtr connection_;

	// Bufferize unreliable messages, which works via UDP.
	// Buffer size - near packet optimal size
	unsigned char unreliable_messages_buffer_[500];
	unsigned int unreliable_messages_buffer_pos_= 0u;
};

} // namespace PanzerChasm
