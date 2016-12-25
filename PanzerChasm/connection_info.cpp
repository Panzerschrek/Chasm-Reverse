#include "connection_info.hpp"

namespace PanzerChasm
{

ConnectionInfo::ConnectionInfo( const IConnectionPtr& in_connection )
	: connection( in_connection )
	, messages_extractor( in_connection )
	, messages_sender( in_connection )
{}

} // namespace PanzerChasm
