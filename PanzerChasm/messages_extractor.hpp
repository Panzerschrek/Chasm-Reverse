#pragma once

#include "fwd.hpp"
#include "i_connection.hpp"
#include "messages.hpp"

namespace PanzerChasm
{

class MessagesExtractor final
{
public:
	explicit MessagesExtractor( IConnectionPtr connection );
	~MessagesExtractor();

	template<class MessagesHandler>
	void ProcessMessages( MessagesHandler& messages_handler );

private:
	static size_t c_messages_size[ size_t(MessageId::NumMessages) ];
	static constexpr unsigned int c_buffer_size= IConnection::c_max_unreliable_packet_size * 2u;

	IConnectionPtr connection_;

	unsigned char reliable_buffer_[ c_buffer_size ];
	unsigned int reliable_buffer_pos_= 0u;

	unsigned char unreliable_buffer_[ c_buffer_size ];
	unsigned int unreliable_buffer_pos_= 0u;
};

} // namespace PanzerChasm
