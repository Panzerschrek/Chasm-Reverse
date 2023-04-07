#include <cstring>

#include "assert.hpp"
#include "i_connection.hpp"
#include "messages.hpp"

#include "messages_extractor.hpp"

namespace PanzerChasm
{

template<class MessagesHandler>
void MessagesExtractor::ProcessMessages( MessagesHandler& messages_handler )
{
	if( broken_ ) return;

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
				{
					// TODO - handel error
					PC_ASSERT( false );
				}

				const unsigned int message_size= c_messages_size[ size_t(message_id) ];
				if( pos + message_size > bytes_to_process )
					break;

				switch(message_id)
				{
				case MessageId::Unknown:
				case MessageId::NumMessages:
					broken_= true;
					return;

				#define MESSAGE_FUNC(x)\
				case MessageId::x:\
					messages_handler( *reinterpret_cast<const Messages::x*>( msg_ptr ) );\
					break;

				#include "messages_list.h"
				#undef MESSAGE_FUNC

				};

				pos+= message_size;
			} // for messages in buffer

			std::memmove( buffer, buffer + pos, bytes_to_process - pos );
			buffer_pos= bytes_to_process - pos;
		}
	}
}

} // namespace PanzerChasm
