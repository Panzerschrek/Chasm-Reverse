#include <cstring>

#include "messages.hpp"

#include "messages_extractor.hpp"

namespace PanzerChasm
{

template<class MessagesHandler>
void MessagesExtractor::ProcessMessages( MessagesHandler& messages_handler )
{
	for( unsigned int i= 0u; i < 2u; i++ ) // for reliable and unreliable messages
	{
		unsigned char* const buffer= i == 0u ? reliable_buffer_ : unreliable_buffer_;
		unsigned int& buffer_pos= i == 0u ? reliable_buffer_pos_ : unreliable_buffer_pos_;
		const unsigned int max_buffer_size= i == 0u ? sizeof(reliable_buffer_) : sizeof(unreliable_buffer_);

		while(1)
		{
			unsigned int bytes_to_process= buffer_pos;
			unsigned int bytes_read= 0u;
			if( i == 0u )
				bytes_read=
					connection_->ReadRealiableData(
						buffer + buffer_pos,
						max_buffer_size - buffer_pos );
			else
				bytes_read=
					connection_->ReadUnrealiableData(
						buffer + buffer_pos,
						max_buffer_size - buffer_pos );

			bytes_to_process+= bytes_read;

			if( bytes_to_process == 0u || bytes_read == 0u )
				break;

			unsigned int pos= 0u;
			while(1)
			{
				if( bytes_to_process - pos < sizeof(MessageId) )
					break;

				const unsigned char* const msg_ptr= buffer + pos;

				MessageId message_id;
				std::memcpy( &message_id, msg_ptr, sizeof(MessageId) );

				if( message_id >= MessageId::NumMessages || message_id <= MessageId::Unknown )
				{ /* TODO - handle error */ }

				const unsigned int message_size= c_messages_size[ size_t(message_id) ];
				if( pos + message_size > bytes_to_process )
					break;

				switch(message_id)
				{
				case MessageId::Unknown:
				case MessageId::NumMessages:
					// TODO - handle error
					PC_ASSERT(false);
					break;

				// Unreliable
				case MessageId::EntityState:
					messages_handler( *reinterpret_cast<const Messages::EntityState*>( msg_ptr ) );
					break;

				case MessageId::WallPosition:
					messages_handler( *reinterpret_cast<const Messages::WallPosition*>( msg_ptr ) );
					break;

				case MessageId::PlayerPosition:
					messages_handler( *reinterpret_cast<const Messages::PlayerPosition*>( msg_ptr ) );
					break;

				// Reliable
				case MessageId::EntityBirth:
					messages_handler( *reinterpret_cast<const Messages::EntityBirth*>( msg_ptr ) );
					break;

				case MessageId::EntityDeath:
					messages_handler( *reinterpret_cast<const Messages::EntityDeath*>( msg_ptr ) );
					break;

				// Unreliable
				case MessageId::PlayerMove:
					messages_handler( *reinterpret_cast<const Messages::PlayerMove*>( msg_ptr ) );
					break;

				// Reliable
				case MessageId::PlayerShot:
					messages_handler( *reinterpret_cast<const Messages::PlayerShot*>( msg_ptr ) );
					break;
				};

				pos+= message_size;
			} // for messages in buffer

			std::memmove( buffer, buffer + pos, bytes_to_process - pos );
			buffer_pos= bytes_to_process - pos;
		}
	}
}

} // namespace PanzerChasm
