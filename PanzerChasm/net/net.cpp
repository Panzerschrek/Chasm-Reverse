#include <winsock2.h>

#include "../assert.hpp"
#include "../log.hpp"

#include "net.hpp"

namespace PanzerChasm
{

struct Net::PlatformData
{
	WSADATA ws_data;
};

Net::Net()
{
	platform_data_.reset( new PlatformData );

#ifdef _WIN32
	WORD version;
	version = MAKEWORD(2, 0);

	const int error_code= WSAStartup( version, &platform_data_->ws_data );
	if( error_code != 0 )
		return;

	Log::Info( "WinSock2 started" );

#else
	// TODO
#endif

	successfully_started_= true;
}

Net::~Net()
{
#ifdef _WIN32
	if( successfully_started_ )
		WSACleanup();
#else
	// TODO
#endif
}

IConnectionPtr Net::ConnectToServer( const InetAddress& address )
{
	PC_UNUSED(address);
	// TODO
	return nullptr;
}

IConnectionsListenerPtr Net::CreateServerListener()
{
	// TODO
	return nullptr;
}

} // namespace PanzerChasm
