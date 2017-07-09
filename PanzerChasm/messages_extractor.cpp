#include "messages_extractor.hpp"

namespace PanzerChasm
{

size_t MessagesExtractor::c_messages_size[ size_t(MessageId::NumMessages) ]=
{
	sizeof(Messages::MessageBase),

	#define MESSAGE_FUNC(x) sizeof(Messages::x),
	#include "messages_list.h"
	#undef MESSAGE_FUNC
};

MessagesExtractor::MessagesExtractor( IConnectionPtr connection )
	: connection_(std::move(connection))
{}

MessagesExtractor::~MessagesExtractor()
{}

} // namespace PanzerChasm
