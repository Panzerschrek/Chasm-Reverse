#pragma once
#include "fwd.hpp"
#include "messages_extractor.hpp"
#include "messages_sender.hpp"

namespace PanzerChasm
{

struct ConnectionInfo
{
	explicit ConnectionInfo( const IConnectionPtr& in_connection );

	const IConnectionPtr connection;
	MessagesExtractor messages_extractor;
	MessagesSender messages_sender;
};

} // namespace PanzerChasm
