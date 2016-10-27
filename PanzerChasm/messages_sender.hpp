#pragma once
#include <type_traits>

#include "i_connection.hpp"
#include "messages.hpp"

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
		static_assert(
			std::is_base_of< Messages::MessageBase, Message >::value,
			"Invalid message type" );

		SendReliableMessageImpl( &message, sizeof(Message) );
	}

	template<class Message>
	void SendUnreliableMessage( const Message& message )
	{
		static_assert(
			std::is_base_of< Messages::MessageBase, Message >::value,
			"Invalid message type" );

		static_assert(
			sizeof(Message) <= sizeof(unreliable_messages_buffer_),
			"Message is too big" );

		SendUnreliableMessageImpl( &message, sizeof(Message) );
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
