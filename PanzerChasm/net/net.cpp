#include <cstring>
#include <vector>

#include <winsock2.h>

#include "../assert.hpp"
#include "../i_connection.hpp"
#include "../log.hpp"
#include "../messages.hpp"
#include "../server/i_connections_listener.hpp"

#include "net.hpp"

namespace PanzerChasm
{

static const uint16_t g_default_port_tcp= 6666u;
static const uint16_t g_default_port_udp= 6667u;

class NetConnection final : public IConnection
{
public:
	NetConnection( const SOCKET& tcp_socket, const SOCKET& udp_socket, const sockaddr_in& destination_udp_address )
		: tcp_socket_( tcp_socket )
		, udp_socket_( udp_socket )
		, destination_udp_address_( destination_udp_address )
	{
		//u_long socket_mode= 1;
		//::ioctlsocket( tcp_socket_, FIONBIO, &socket_mode );
		//::ioctlsocket( in_udp_socket_, FIONBIO, &socket_mode );
		//::ioctlsocket( out_udp_socket_, FIONBIO, &socket_mode );
	}

	virtual ~NetConnection() override
	{
		::closesocket( tcp_socket_ );
		::closesocket( udp_socket_ );
	}

public: // IConnection
	virtual void SendReliablePacket( const void* data, unsigned int data_size ) override
	{
		::send( tcp_socket_, (const char*) data, data_size, 0 );
	}

	virtual void SendUnreliablePacket( const void* data, unsigned int data_size ) override
	{
		::sendto( udp_socket_, (const char*) data, data_size, 0, (sockaddr*) &destination_udp_address_, sizeof(destination_udp_address_) );
	}

	virtual unsigned int ReadRealiableData( void* out_data, unsigned int buffer_size ) override
	{
		fd_set set;
		set.fd_count= 1u;
		set.fd_array[0]= tcp_socket_;

		timeval wait_time;
		wait_time.tv_sec= 0u;
		wait_time.tv_usec= 1000u;

		unsigned int socket_ready= ::select( 0, &set, nullptr, nullptr, &wait_time );
		// TODO - check error
		if( socket_ready == 1u )
		{
			int result= ::recv( tcp_socket_, (char*) out_data, buffer_size, 0 );
			if( result == SOCKET_ERROR )
			{
				Log::Info( "Error: ", ::WSAGetLastError() );
			}
			return std::max( 0, result );
		}
		return 0u;
	}

	virtual unsigned int ReadUnrealiableData( void* out_data, unsigned int buffer_size ) override
	{
		fd_set set;
		set.fd_count= 1u;
		set.fd_array[0]= udp_socket_;

		timeval wait_time;
		wait_time.tv_sec= 0u;
		wait_time.tv_usec= 1000u;

		unsigned int socket_ready= ::select( 0, &set, nullptr, nullptr, &wait_time );
		// TODO - check error
		if( socket_ready == 1u )
		{
			sockaddr_in reciever_address;
			int reciever_address_length= sizeof(reciever_address);
			// TODO - check reciever_address
			int result=
				::recvfrom( udp_socket_, (char*) out_data, buffer_size, 0, (sockaddr*) &reciever_address, &reciever_address_length );

			if( result == SOCKET_ERROR )
			{
				Log::Info( "Error: ", ::WSAGetLastError() );
				return 0u;
			}

			return std::max( result, 0 );
		}
		return 0;
	}

	virtual void Disconnect() override
	{
		// TODO
	}
	virtual bool Disconnected() override
	{
		// TODO
		return false;
	}

private:
	const SOCKET tcp_socket_= INVALID_SOCKET;
	const SOCKET udp_socket_= INVALID_SOCKET;
	const sockaddr_in destination_udp_address_;
};

class EstablishingConnection
{
public:
	EstablishingConnection( const SOCKET tcp_socket, const uint16_t udp_port )
		: tcp_socket_(tcp_socket)
	{
		// Send to client input udp address, wia tcp.
		::send( tcp_socket_, (char*) &udp_port, sizeof(udp_port), 0 ); // TODO - check errors.

		udp_socket_= ::socket( AF_INET, SOCK_DGRAM, 0 );
		if( udp_socket_ == INVALID_SOCKET )
		{
			Log::Warning( "Can not create udp socket. Error code: ", ::WSAGetLastError() );
			// TODO - to something.
			return;
		}

		sockaddr_in udp_address;
		udp_address.sin_family= AF_INET;
		udp_address.sin_addr.s_addr= INADDR_ANY; //::inet_addr( "127.0.0.1" );
		udp_address.sin_port= ::htons( udp_port );
		const int bind_result= ::bind( udp_socket_, (sockaddr*) &udp_address, sizeof(udp_address) );
		if( bind_result != 0 )
		{
			Log::Warning( "Can not bind udp socket. Error code: ", ::WSAGetLastError() );
			return;
		}
	}

	IConnectionPtr TryCompleteConnection()
	{
		fd_set set;
		set.fd_count= 1u; set.fd_array[0]= udp_socket_;

		timeval wait_time;
		wait_time.tv_sec= 0u; wait_time.tv_usec= 1000u;

		unsigned int socket_ready= ::select( 0, &set, nullptr, nullptr, &wait_time );
		// TODO - check error
		if( socket_ready == 1u )
		{
			sockaddr_in reciever_address;
			int reciever_address_length= sizeof(reciever_address);

			// Recieve any message from client to estabelishing of connection.
			// Use MSG_PEEK, because we not want to dump really meaning message.
			unsigned char dummy_buffer[ IConnection::c_max_unreliable_packet_size ];
			int result=
				::recvfrom(
					udp_socket_,
					(char*) &dummy_buffer, sizeof(dummy_buffer),
					MSG_PEEK,
					(sockaddr*) &reciever_address, &reciever_address_length );

			// TODO - check reciever_address (must match IP address with tcp connection).
			if( result == SOCKET_ERROR )
			{
				Log::Info( "Error: ", ::WSAGetLastError() );
				return nullptr;
			}

			const SOCKET tcp_socket= tcp_socket_; tcp_socket_= INVALID_SOCKET;
			const SOCKET udp_socket= udp_socket_; udp_socket_= INVALID_SOCKET;
			return std::make_shared<NetConnection>( tcp_socket, udp_socket, reciever_address );
		}

		return nullptr;
	}

private:
	SOCKET tcp_socket_= INVALID_SOCKET;
	SOCKET udp_socket_= INVALID_SOCKET;
};

typedef std::unique_ptr<EstablishingConnection> EstablishingConnectionPtr;

class ServerListener final : public IConnectionsListener
{
public:
	ServerListener(
		const uint16_t tcp_port,
		const uint16_t base_udp_port )
		: listen_port_( tcp_port )
		, next_in_udp_port_( base_udp_port )
	{
		listen_socket_= ::socket( PF_INET, SOCK_STREAM, 0 );
		if( listen_socket_ == INVALID_SOCKET )
		{
			Log::Warning( "Can not create tcp socket. Error code: ", ::WSAGetLastError() );
			return;
		}

		sockaddr_in listen_socket_address;
		std::memset( &listen_socket_address, 0, sizeof(listen_socket_address) );
		listen_socket_address.sin_family = AF_INET;
		listen_socket_address.sin_addr.s_addr = INADDR_ANY;//::inet_addr( "0.0.0.0" ); // TODO - maybe INADDR_ANY?
		listen_socket_address.sin_port= ::htons( listen_port_ );
		const int bind_result= ::bind( listen_socket_, (sockaddr*) &listen_socket_address, sizeof(listen_socket_address) );
		if( bind_result != 0 )
		{
			Log::Warning( "Can not bind listen socket. Error code: ", ::WSAGetLastError() );
			return;
		}

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
		// Ty complete establishing connections.
		for( unsigned int c= 0u; c < establishing_connections_.size(); c++ )
		{
			if( const IConnectionPtr connection= establishing_connections_[c]->TryCompleteConnection() )
			{
				if( c != establishing_connections_.size() - 1u )
					establishing_connections_[c]= std::move( establishing_connections_.back() );
				establishing_connections_.pop_back();
				return connection;
			}
		}

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

			Log::Info( "Client connected to server" );

			const uint16_t connection_in_udp_port= next_in_udp_port_;
			++next_in_udp_port_;

			establishing_connections_.emplace_back( new EstablishingConnection( client_tcp_socket, connection_in_udp_port ) );
		}

		return nullptr;
	}

private:
	SOCKET listen_socket_= INVALID_SOCKET;
	const uint16_t listen_port_;
	uint16_t next_in_udp_port_;
	bool all_ok_= false;

	std::vector< EstablishingConnectionPtr> establishing_connections_;
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

IConnectionPtr Net::ConnectToServer( const InetAddress& address, uint16_t in_udp_port )
{
	PC_UNUSED(address);

	const SOCKET tcp_socket= ::socket( AF_INET, SOCK_STREAM, 0 );
	if( tcp_socket == INVALID_SOCKET )
	{
		Log::Warning( "Can not create tcp socket. Error code: ", ::WSAGetLastError() );
		return nullptr;
	}

	sockaddr_in server_tcp_address;
	std::memset( &server_tcp_address, 0, sizeof(server_tcp_address) );
	server_tcp_address.sin_family= AF_INET;
	server_tcp_address.sin_addr.s_addr= inet_addr( "127.0.0.1" );
	server_tcp_address.sin_port= ::htons( address.port );

	const SOCKET udp_socket= ::socket( AF_INET, SOCK_DGRAM, 0 );
	if( tcp_socket == INVALID_SOCKET )
	{
		Log::Warning( "Can not create udp socket. Error code: ", ::WSAGetLastError() );
		return nullptr;
	}

	sockaddr_in udp_address;
	udp_address.sin_family= AF_INET;
	udp_address.sin_addr.s_addr= INADDR_ANY; //::inet_addr( "127.0.0.1" );
	udp_address.sin_port= ::htons( in_udp_port );
	::bind( udp_socket, (sockaddr*) &udp_address, sizeof(udp_address) );

	const int connection_result= ::connect( tcp_socket, (sockaddr*) &server_tcp_address, sizeof(server_tcp_address) );
	if( connection_result == SOCKET_ERROR )
	{
		Log::Warning( "Can not connect to server. Error code: ", ::WSAGetLastError() );
		return nullptr;
	}

	// Recive from server it input udp address.
	uint16_t server_udp_port;
	::recv( tcp_socket, (char*) &server_udp_port, sizeof(server_udp_port), 0 ); // TODO - check errors.
	sockaddr_in server_udp_address;
	std::memcpy( &server_udp_address, &server_tcp_address, sizeof(sockaddr_in) );
	server_udp_address.sin_port= ::htons( server_udp_port );

	// Send to server first udp message for establishing of connection.
	// Make NAT happy.
	// Make this multiple times, for better reliability.
	// We can just start send real game messages, but first message, that server catches, will be lost.
	for( unsigned int n= 0u; n < 4u; n++ )
	{
		Messages::DummyNetMessage first_message;
		::sendto( udp_socket, (char*) &first_message, sizeof(first_message), 0, (sockaddr*) &server_udp_address, sizeof(server_udp_address) );
	}

	return std::make_shared<NetConnection>( tcp_socket, udp_socket, server_udp_address );
}

IConnectionsListenerPtr Net::CreateServerListener(
	const uint16_t tcp_port,
	const uint16_t base_udp_port )
{
	const auto listener= std::make_shared<ServerListener>( tcp_port, base_udp_port );

	if( listener->IsOk() )
		return listener;

	return nullptr;
}

} // namespace PanzerChasm
