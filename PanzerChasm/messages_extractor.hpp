#pragma once

#include "fwd.hpp"
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

	IConnectionPtr connection_;

	unsigned char reliable_buffer_[500];
	unsigned int reliable_buffer_pos_= 0u;

	unsigned char unreliable_buffer_[500];
	unsigned int unreliable_buffer_pos_= 0u;
};

} // namespace PanzerChasm
