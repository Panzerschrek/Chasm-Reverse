#include "messages_extractor.hpp"

namespace PanzerChasm
{

size_t MessagesExtractor::c_messages_size[ size_t(MessageId::NumMessages) ]=
{
	[size_t(MessageId::Unknown)]= sizeof(Messages::MessageBase),

	#define MESSAGE_FUNC(x) [size_t(MessageId::x)]= sizeof(Messages::x),
	#include "messages_list.h"
	#undef MESSAGE_FUNC
};

MessagesExtractor::MessagesExtractor( IConnectionPtr connection )
	: connection_(std::move(connection))
{}

MessagesExtractor::~MessagesExtractor()
{}

} // namespace PanzerChasm
