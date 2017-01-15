#include <cstring>

#include <winsock2.h>

#include "../assert.hpp"
#include "../i_connection.hpp"
#include "../log.hpp"
#include "../server/i_connections_listener.hpp"

#include "net.hpp"

namespace PanzerChasm
{

static const uint16_t g_default_port_tcp= 6666u;
static const uint16_t g_default_port_udp= 6667u;

class NetConnection final : public IConnection
{
public:
	NetConnection( const SOCKET& tcp_socket, const sockaddr_in& destianation_address )
	{
		tcp_socket_= tcp_socket;
		destination_address_= destianation_address;

		in_udp_socket_= ::socket( PF_INET, SOCK_STREAM, 0 );
		if( in_udp_socket_ == INVALID_SOCKET )
		{
			Log::Warning( "Can not create udp socket. Error code: ", ::WSAGetLastError() );
			return;
		}

		sockaddr_in in_udp_address;
		std::memset( &in_udp_address, 0, sizeof(in_udp_address) );
		in_udp_address.sin_addr.s_addr = ::inet_addr( "127.0.0.1" ); // TODO - maybe INADDR_ANY?
		in_udp_address.sin_port= ::htons( g_default_port_udp );
		::bind( in_udp_socket_, (sockaddr*) &in_udp_address, sizeof(in_udp_address) );

		out_udp_socket_= ::socket( PF_INET, SOCK_STREAM, 0 );
		if( out_udp_socket_ == INVALID_SOCKET )
		{
			Log::Warning( "Can not create udp socket. Error code: ", ::WSAGetLastError() );
			return;
		}
	}

	virtual ~NetConnection() override
	{
		::closesocket( tcp_socket_ );

		::closesocket( in_udp_socket_ );
		::closesocket( out_udp_socket_ );
	}

public: // IConnection
	virtual void SendReliablePacket( const void* data, unsigned int data_size ) override
	{
		::send( tcp_socket_, (const char*) data, data_size, 0 );
	}

	virtual void SendUnreliablePacket( const void* data, unsigned int data_size ) override
	{
		::sendto( out_udp_socket_, (const char*) data, data_size, 0, (sockaddr*) &destination_address_, sizeof(destination_address_) );
	}

	virtual unsigned int ReadRealiableData( void* out_data, unsigned int buffer_size ) override
	{
		return ::recv( tcp_socket_, (char*) out_data, buffer_size, 0 );
	}

	virtual unsigned int ReadUnrealiableData( void* out_data, unsigned int buffer_size ) override
	{
		sockaddr_in reciever_address;
		int reciever_address_length= sizeof(reciever_address);
		// TODO - check reciever_address
		return ::recvfrom( in_udp_socket_, (char*) out_data, buffer_size, 0, (sockaddr*) &reciever_address, &reciever_address_length );
	}

	virtual void Disconnect() override
	{
		// TODO
	}
	virtual bool Disconnected()override
	{
		// TODO
		return false;
	}

private:
	SOCKET tcp_socket_= INVALID_SOCKET;

	SOCKET in_udp_socket_= INVALID_SOCKET;
	SOCKET out_udp_socket_= INVALID_SOCKET;

	sockaddr_in destination_address_;
};

class ServerListener final : public IConnectionsListener
{
public:
	ServerListener()
	{
		listen_socket_= ::socket( PF_INET, SOCK_STREAM, 0 );
		if( listen_socket_ == INVALID_SOCKET )
		{
			Log::Warning( "Can not create tcp socket. Error code: ", ::WSAGetLastError() );
			return;
		}

		sockaddr_in listen_socket_address;
		std::memset( &listen_socket_address, 0, sizeof(listen_socket_address) );
		listen_socket_address.sin_addr.s_addr = ::inet_addr( "127.0.0.1" ); // TODO - maybe INADDR_ANY?
		listen_socket_address.sin_port= ::htons( g_default_port_tcp );
		::bind( listen_socket_, (sockaddr*) &listen_socket_address, sizeof(listen_socket_address) );

		const int listen_result= ::listen( listen_socket_, SOMAXCONN );
		if( listen_result != 0 )
		{
			Log::Warning( "Can not start listen. Error code: ", ::WSAGetLastError() );
			return;
		}

		all_ok_= true;
	}

	~ServerListener()
	{
		if( listen_socket_ != INVALID_SOCKET )
			::closesocket( listen_socket_ );
	}

	bool IsOk() const
	{
		return all_ok_;
	}

public: // IConnectionsListener
	virtual IConnectionPtr GetNewConnection() override
	{
		fd_set set;
		set.fd_count= 1u;
		set.fd_array[0]= listen_socket_;

		timeval wait_time;
		wait_time.tv_sec= 0u;
		wait_time.tv_usec= 1000u;

		unsigned int socket_ready= ::select( 0, &set, nullptr, nullptr, &wait_time );
		// TODO - check error
		if( socket_ready == 1u )
		{
			// Create connection.
			sockaddr_in client_address;
			int client_address_len= sizeof(client_address);
			const SOCKET client_tcp_socket=
				::accept( listen_socket_, (sockaddr*) &client_address, &client_address_len );

			if( client_tcp_socket == INVALID_SOCKET )
			{
				Log::Warning( "Can not accept client. Error code: ", ::WSAGetLastError() );
				return nullptr;
			}

			return std::make_shared<NetConnection>( client_tcp_socket, client_address );
		}

		return nullptr;
	}

private:
	SOCKET listen_socket_= INVALID_SOCKET;
	bool all_ok_= false;
};

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

	const int error_code= ::WSAStartup( version, &platform_data_->ws_data );
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
		::WSACleanup();
#else
	// TODO
#endif
}

IConnectionPtr Net::ConnectToServer( const InetAddress& address )
{
	PC_UNUSED(address);

	const SOCKET tmp_socket= ::socket( AF_INET, SOCK_STREAM, 0 );
	if( tmp_socket == INVALID_SOCKET )
	{
		Log::Warning( "Can not create tcp socket. Error code: ", ::WSAGetLastError() );
		return nullptr;
	}

	sockaddr_in server_address;
	std::memset( &server_address, 0, sizeof(server_address) );
	server_address.sin_family= AF_INET;
	server_address.sin_addr.s_addr= inet_addr( "127.0.0.1" );
	server_address.sin_port= ::htons( g_default_port_tcp );

	const SOCKET connection_socket= ::connect( tmp_socket, (sockaddr*) &server_address, sizeof(server_address) );
	if( connection_socket == INVALID_SOCKET )
	{
		Log::Warning( "Can not connect to server. Error code: ", ::WSAGetLastError() );
		return nullptr;
	}

	::closesocket( tmp_socket );

	return std::make_shared<NetConnection>( connection_socket, server_address );
}

IConnectionsListenerPtr Net::CreateServerListener()
{
	const auto listener= std::make_shared<ServerListener>();

	if( listener->IsOk() )
		return listener;

	return nullptr;
}

} // namespace PanzerChasm
