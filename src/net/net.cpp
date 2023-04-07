#include <cctype>
#include <cstring>
#include <vector>

#ifdef _WIN32
#ifdef _MSC_VER
#define NOMINMAX
#endif // _MSC_VER
#include <winsock2.h>
#else

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define INVALID_SOCKET (-1)
typedef int SOCKET;

#endif

#include "../assert.hpp"
#include "../i_connection.hpp"
#include "../log.hpp"
#include "../messages.hpp"
#include "../server/i_connections_listener.hpp"

#include "net.hpp"

#define FUNC_NAME  __FUNCTION__

namespace PanzerChasm
{

typedef uint32_t IpAddress; // Ip address in net byte order.

static bool IsSocketReady( const SOCKET& socket )
{
#ifdef _WIN32
	fd_set set;
	set.fd_count= 1u;
	set.fd_array[0]= socket;

	timeval wait_time;
	wait_time.tv_sec= 0u;
	wait_time.tv_usec= 0u;

	const int result= ::select( 0, &set, nullptr, nullptr, &wait_time );
	if( result == 1 )
		return true;
	else if( result == 0 )
		return false;
	else
	{
		Log::Warning( FUNC_NAME, " -  ::select call error: ", ::WSAGetLastError() );
		return false;
	}
#else
	fd_set set;
	FD_ZERO(&set);
	FD_SET( socket, &set );

	timeval wait_time;
	wait_time.tv_sec= 0u;
	wait_time.tv_usec= 0u;

	const int result= ::select( socket + 1u, &set, nullptr, nullptr, &wait_time );
	if( result == 1 )
		return true;
	else if( result == 0 )
		return false;
	else
	{
		Log::Warning( FUNC_NAME, " -  ::select call error: ", errno );
		return false;
	}
#endif
}

bool InetAddress::Parse( const std::string& address_string, InetAddress& out_address )
{
	unsigned char addr[4];

	const char* str= address_string.c_str();
	for( unsigned int i= 0u; i < 4u; i++ )
	{
		if( ! std::isdigit( *str ) ) return false;

		const int n= std::atoi(str);
		if( ! ( n >= 0 && n <= 255 ) ) return false;

		addr[ 3u - i ]= static_cast<unsigned char>(n);

		while( std::isdigit( *str ) && *str != '\0' ) str++;

		if( i != 3u )
		{
			if( *str != '.' ) return false;
			str++;
		}
	}

	std::memcpy( &out_address.ip_address, &addr, 4u );
	out_address.port= 0u;

	if( *str == ':' )
	{
		str++;
		if( *str == '\0' ) return false;
		if( ! std::isdigit( *str ) ) return false;
		const int port= std::atoi( str );
		if( !( port > 0 && port < 65536 ) ) return false;
		out_address.port= port;
	}

	return true;
}

std::string InetAddress::ToString() const
{
	std::string result;
	result.reserve( std::strlen( "255.255.255.255:65535") );

	in_addr ip;
#ifdef _WIN32
	ip.S_un.S_addr= ::htonl( ip_address );
#else
	ip.s_addr= htonl( ip_address );
#endif

	result+= ::inet_ntoa( ip );
	result+= ":" + std::to_string( port );

	return result;
}

class NetConnection final : public IConnection
{
public:
	NetConnection( const SOCKET& tcp_socket, const SOCKET& udp_socket, const sockaddr_in& destination_udp_address )
		: tcp_socket_( tcp_socket )
		, udp_socket_( udp_socket )
		, destination_udp_address_( destination_udp_address )
	{
		// TEST - use nonblocking sockets.
		//u_long socket_mode= 1;
		//::ioctlsocket( tcp_socket_, FIONBIO, &socket_mode );
		//::ioctlsocket( udp_socket_, FIONBIO, &socket_mode );
	}

	virtual ~NetConnection() override
	{
		Disconnect();
#ifdef _WIN32
		::closesocket( tcp_socket_ );
		::closesocket( udp_socket_ );
#else
		::close( tcp_socket_ );
		::close( udp_socket_ );
#endif
	}

public: // IConnection
	virtual void SendReliablePacket( const void* data, unsigned int data_size ) override
	{
		if( disconnected_ ) return;
		if( data_size == 0u ) return;

#ifdef _WIN32
		const int result= ::send( tcp_socket_, (const char*) data, data_size, 0 );
		if( result == SOCKET_ERROR )
			Log::Warning( FUNC_NAME, " error: ", ::WSAGetLastError() );
#else
		const int result= ::send( tcp_socket_, (const char*) data, data_size, 0 );
		if( result == -1 )
			Log::Warning( FUNC_NAME, " error: ", errno );
#endif
	}

	virtual void SendUnreliablePacket( const void* data, unsigned int data_size ) override
	{
		if( disconnected_ ) return;

#ifdef _WIN32
		const int result=
			::sendto( udp_socket_, (const char*) data, data_size, 0, (sockaddr*) &destination_udp_address_, sizeof(destination_udp_address_) );

		if( result == SOCKET_ERROR )
			Log::Warning( FUNC_NAME, " error: ", ::WSAGetLastError() );
		else if( result < static_cast<int>(data_size) )
			Log::Warning( FUNC_NAME, " not all data transmited: ", result, " from ", data_size );
#else
		const int result=
			::sendto( udp_socket_, (const char*) data, data_size, 0, (sockaddr*) &destination_udp_address_, sizeof(destination_udp_address_) );

		if( result == -1 )
			Log::Warning( FUNC_NAME, " error: ", errno );
		else if( result < static_cast<int>(data_size) )
			Log::Warning( FUNC_NAME, " not all data transmited: ", result, " from ", data_size );
#endif
	}

	virtual unsigned int ReadRealiableData( void* out_data, unsigned int buffer_size ) override
	{
		if( disconnected_ ) return 0u;

		if( IsSocketReady( tcp_socket_ ) )
		{
#ifdef _WIN32
			int result= ::recv( tcp_socket_, (char*) out_data, buffer_size, 0 );
			if( result == SOCKET_ERROR )
				Log::Warning( FUNC_NAME, " error: ", ::WSAGetLastError() );
#else
			int result= ::recv( tcp_socket_, (char*) out_data, buffer_size, 0 );
			if( result == -1 )
				Log::Warning( FUNC_NAME, " error: ", errno );
#endif
			// If socket is ready, but recv return zero, this means, that other side closes connection.
			if( result == 0 )
				Disconnect();

			return std::max( 0, result );
		}
		return 0u;
	}

	virtual unsigned int ReadUnrealiableData( void* out_data, unsigned int buffer_size ) override
	{
		if( disconnected_ ) return 0u;

		if( IsSocketReady( udp_socket_ ) )
		{
#ifdef _WIN32
			sockaddr_in reciever_address;
			int reciever_address_length= sizeof(reciever_address);
			int result=
				::recvfrom( udp_socket_, (char*) out_data, buffer_size, 0, (sockaddr*) &reciever_address, &reciever_address_length );

			if( result == SOCKET_ERROR )
			{
				Log::Warning( FUNC_NAME, " error: ", ::WSAGetLastError() );
				return 0u;
			}

			if( !( // Check for correct addres - discard messages from invalid address.
				reciever_address.sin_addr.S_un.S_addr == destination_udp_address_.sin_addr.S_un.S_addr &&
				reciever_address.sin_port == destination_udp_address_.sin_port ) )
			{
				return 0u;
			}
#else
			sockaddr_in reciever_address;
			socklen_t reciever_address_length= sizeof(reciever_address);
			int result=
				::recvfrom( udp_socket_, (char*) out_data, buffer_size, 0, (sockaddr*) &reciever_address, &reciever_address_length );

			if( result == -1 )
			{
				Log::Warning( FUNC_NAME, " error: ", errno );
				return 0u;
			}

			if( !( // Check for correct addres - discard messages from invalid address.
				reciever_address.sin_addr.s_addr == destination_udp_address_.sin_addr.s_addr &&
				reciever_address.sin_port == destination_udp_address_.sin_port ) )
			{
				return 0u;
			}
#endif

			return std::max( result, 0 );
		}
		return 0;
	}

	virtual void Disconnect() override
	{
		if( disconnected_ ) return;
		disconnected_= true;

#ifdef _WIN32
		if( ::shutdown( tcp_socket_, SD_BOTH ) != 0 )
			Log::Warning( FUNC_NAME, " error, during closing tcp connection: ", ::WSAGetLastError() );
		if( ::shutdown( udp_socket_, SD_BOTH ) != 0 )
			Log::Warning( FUNC_NAME, " error, during closing udp connection: ", ::WSAGetLastError() );
#else
		if( ::shutdown( tcp_socket_, SHUT_RDWR ) != 0 )
			Log::Warning( FUNC_NAME, " error, during closing tcp connection: ", errno );
		if( ::shutdown( udp_socket_, SHUT_RDWR ) != 0 )
			Log::Warning( FUNC_NAME, " error, during closing udp connection: ", errno );
#endif
	}

	virtual bool Disconnected() override
	{
		return disconnected_;
	}

	virtual std::string GetConnectionInfo() override
	{
		std::string result;
		result.reserve( std::strlen( "255.255.255.255:65535") );

		result+= ::inet_ntoa( destination_udp_address_.sin_addr );
		result+= ":" + std::to_string( ntohs( destination_udp_address_.sin_port ) );

		return result;
	}

private:
	const SOCKET tcp_socket_= INVALID_SOCKET;
	const SOCKET udp_socket_= INVALID_SOCKET;
	const sockaddr_in destination_udp_address_;

	bool disconnected_= false;
};

class EstablishingConnection
{
public:
	EstablishingConnection(
		const SOCKET tcp_socket,
		const IpAddress client_ip_address,
		const uint16_t udp_port )
		: tcp_socket_(tcp_socket)
		, client_ip_address_(client_ip_address)
	{
#ifdef _WIN32
		// Send to client protocol version, wia tcp.
		uint32_t protocol_version= Messages::c_protocol_version;
		::send( tcp_socket_, (char*) &protocol_version, sizeof(protocol_version), 0 ); // TODO - check errors.

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
		udp_address.sin_addr.s_addr= INADDR_ANY;
		udp_address.sin_port= ::htons( udp_port );
		const int bind_result= ::bind( udp_socket_, (sockaddr*) &udp_address, sizeof(udp_address) );
		if( bind_result != 0 )
		{
			Log::Warning( FUNC_NAME, " can not bind udp socket. Error code: ", ::WSAGetLastError() );
			return;
		}
#else
		// Send to client protocol version, wia tcp.
		uint32_t protocol_version= Messages::c_protocol_version;
		::send( tcp_socket_, (char*) &protocol_version, sizeof(protocol_version), 0 ); // TODO - check errors.

		// Send to client input udp address, wia tcp.
		::send( tcp_socket_, (char*) &udp_port, sizeof(udp_port), 0 ); // TODO - check errors.

		udp_socket_= ::socket( AF_INET, SOCK_DGRAM, 0 );
		if( udp_socket_ == -1 )
		{
			Log::Warning( "Can not create udp socket. Error code: ", errno );
			// TODO - to something.
			return;
		}

		sockaddr_in udp_address;
		udp_address.sin_family= AF_INET;
		udp_address.sin_addr.s_addr= INADDR_ANY;
		udp_address.sin_port= htons( udp_port );
		const int bind_result= ::bind( udp_socket_, (sockaddr*) &udp_address, sizeof(udp_address) );
		if( bind_result != 0 )
		{
			Log::Warning( FUNC_NAME, " can not bind udp socket. Error code: ", errno );
			return;
		}
#endif
	}

	IConnectionPtr TryCompleteConnection()
	{
		if( !IsSocketReady( udp_socket_ ) )
			return nullptr;

		// Recieve any message from client to estabelishing of connection.
		// Use MSG_PEEK, because we not want to dump really meaning message.
		unsigned char dummy_buffer[ IConnection::c_max_unreliable_packet_size ];

		sockaddr_in reciever_address;
#ifdef _WIN32
		int reciever_address_length= sizeof(reciever_address);
		int result=
			::recvfrom(
				udp_socket_,
				(char*) &dummy_buffer, sizeof(dummy_buffer),
				MSG_PEEK,
				(sockaddr*) &reciever_address, &reciever_address_length );

		if( result == SOCKET_ERROR )
		{
			Log::Warning( FUNC_NAME, " error: ", ::WSAGetLastError() );
			return nullptr;
		}

		if( reciever_address.sin_addr.S_un.S_addr != client_ip_address_ )
		{
			Log::Info( "Unknown user ", inet_ntoa( reciever_address.sin_addr ), " trying to connect. Discard him." );
			return nullptr;
		}
#else
		socklen_t reciever_address_length= sizeof(reciever_address);
		int result=
			::recvfrom(
				udp_socket_,
				(char*) &dummy_buffer, sizeof(dummy_buffer),
				MSG_PEEK,
				(sockaddr*) &reciever_address, &reciever_address_length );

		if( result == -1 )
		{
			Log::Warning( FUNC_NAME, " error: ", errno );
			return nullptr;
		}

		if( reciever_address.sin_addr.s_addr != client_ip_address_ )
		{
			Log::Info( "Unknown user ", inet_ntoa( reciever_address.sin_addr ), " trying to connect. Discard him." );
			return nullptr;
		}
#endif

		const SOCKET tcp_socket= tcp_socket_; tcp_socket_= INVALID_SOCKET;
		const SOCKET udp_socket= udp_socket_; udp_socket_= INVALID_SOCKET;
		return std::make_shared<NetConnection>( tcp_socket, udp_socket, reciever_address );
	}

private:
	SOCKET tcp_socket_= INVALID_SOCKET;
	SOCKET udp_socket_= INVALID_SOCKET;
	const IpAddress client_ip_address_;
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
#ifdef _WIN32
		listen_socket_= ::socket( PF_INET, SOCK_STREAM, 0 );
		if( listen_socket_ == INVALID_SOCKET )
		{
			Log::Warning( "Can not create tcp socket. Error code: ", ::WSAGetLastError() );
			return;
		}

		sockaddr_in listen_socket_address;
		std::memset( &listen_socket_address, 0, sizeof(listen_socket_address) );
		listen_socket_address.sin_family = AF_INET;
		listen_socket_address.sin_addr.s_addr = INADDR_ANY;
		listen_socket_address.sin_port= ::htons( listen_port_ );
		const int bind_result= ::bind( listen_socket_, (sockaddr*) &listen_socket_address, sizeof(listen_socket_address) );
		if( bind_result != 0 )
		{
			Log::Warning( "Can not bind listen socket. Error code: ", ::WSAGetLastError() );
			::closesocket( listen_socket_ );
			return;
		}

		const int listen_result= ::listen( listen_socket_, SOMAXCONN );
		if( listen_result != 0 )
		{
			Log::Warning( "Can not start listen. Error code: ", ::WSAGetLastError() );
			::closesocket( listen_socket_ );
			return;
		}
#else
		listen_socket_= ::socket( PF_INET, SOCK_STREAM, 0 );
		if( listen_socket_ == -1 )
		{
			Log::Warning( "Can not create tcp socket. Error code: ", errno );
			return;
		}

		sockaddr_in listen_socket_address;
		std::memset( &listen_socket_address, 0, sizeof(listen_socket_address) );
		listen_socket_address.sin_family = AF_INET;
		listen_socket_address.sin_addr.s_addr = INADDR_ANY;
		listen_socket_address.sin_port= htons( listen_port_ );
		const int bind_result= ::bind( listen_socket_, (sockaddr*) &listen_socket_address, sizeof(listen_socket_address) );
		if( bind_result != 0 )
		{
			Log::Warning( "Can not bind listen socket. Error code: ", errno );
			::close( listen_socket_ );
			return;
		}

		const int listen_result= ::listen( listen_socket_, SOMAXCONN );
		if( listen_result != 0 )
		{
			Log::Warning( "Can not start listen. Error code: ", errno );
			::close( listen_socket_ );
			return;
		}
#endif

		all_ok_= true;
	}

	~ServerListener()
	{
#ifdef _WIN32
		if( listen_socket_ != INVALID_SOCKET )
			::closesocket( listen_socket_ );
#else
		if( listen_socket_ != INVALID_SOCKET )
			::close( listen_socket_ );
#endif
	}

	bool IsOk() const
	{
		return all_ok_;
	}

public: // IConnectionsListener
	virtual IConnectionPtr GetNewConnection() override
	{
		if( IsSocketReady( listen_socket_ ) )
		{
#ifdef _WIN32
			sockaddr_in client_address;
			int client_address_len= sizeof(client_address);
			const SOCKET client_tcp_socket=
				::accept( listen_socket_, (sockaddr*) &client_address, &client_address_len );

			if( client_tcp_socket == INVALID_SOCKET )
			{
				Log::Warning( "Can not accept client. Error code: ", ::WSAGetLastError() );
				return nullptr;
			}

			const IpAddress client_ip_address= client_address.sin_addr.S_un.S_addr;
#else
			sockaddr_in client_address;
			socklen_t client_address_len= sizeof(client_address);
			const SOCKET client_tcp_socket=
				::accept( listen_socket_, (sockaddr*) &client_address, &client_address_len );

			if( client_tcp_socket == -1 )
			{
				Log::Warning( "Can not accept client. Error code: ", errno );
				return nullptr;
			}

			const IpAddress client_ip_address= client_address.sin_addr.s_addr;
#endif
			const uint16_t connection_in_udp_port= next_in_udp_port_;
			++next_in_udp_port_;

			establishing_connections_.emplace_back(
			new EstablishingConnection(
				client_tcp_socket,
				client_ip_address,
				connection_in_udp_port ) );
		}

		// Try complete establishing connections.
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
#ifdef _WIN32
	WSADATA ws_data;
#else
#endif
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

IConnectionPtr Net::ConnectToServer(
	const InetAddress& address,
	const uint16_t in_udp_port,
	const uint16_t in_tcp_port )
{
#ifdef _WIN32
	// Create and open TCP socket.
	const SOCKET tcp_socket= ::socket( AF_INET, SOCK_STREAM, 0 );
	if( tcp_socket == INVALID_SOCKET )
	{
		Log::Warning( FUNC_NAME, " can not create tcp socket. Error code: ", ::WSAGetLastError() );
		return nullptr;
	}

	sockaddr_in tcp_address;
	tcp_address.sin_family= AF_INET;
	tcp_address.sin_addr.s_addr= INADDR_ANY;
	tcp_address.sin_port= ::htons( in_tcp_port );
	if( ::bind( tcp_socket, (sockaddr*) &tcp_address, sizeof(tcp_address) ) != 0 )
	{
		Log::Warning( FUNC_NAME, " can not bind tcp socket. Error code: ", ::WSAGetLastError() );
		::closesocket( tcp_socket );
		return nullptr;
	}

	// Createe and open UDP socket.
	const SOCKET udp_socket= ::socket( AF_INET, SOCK_DGRAM, 0 );
	if( udp_socket == INVALID_SOCKET )
	{
		Log::Warning( FUNC_NAME, " can not create udp socket. Error code: ", ::WSAGetLastError() );
		::closesocket( tcp_socket );
		return nullptr;
	}

	sockaddr_in udp_address;
	udp_address.sin_family= AF_INET;
	udp_address.sin_addr.s_addr= INADDR_ANY;
	udp_address.sin_port= ::htons( in_udp_port );
	if( ::bind( udp_socket, (sockaddr*) &udp_address, sizeof(udp_address) ) != 0 )
	{
		Log::Warning( FUNC_NAME, " can not bind udp socket. Error code: ", ::WSAGetLastError() );
		::closesocket( tcp_socket );
		::closesocket( udp_socket );
		return nullptr;
	}

	// Connect to server.
	sockaddr_in server_tcp_address;
	std::memset( &server_tcp_address, 0, sizeof(server_tcp_address) );
	server_tcp_address.sin_family= AF_INET;
	server_tcp_address.sin_addr.s_addr= ::htonl( address.ip_address );
	server_tcp_address.sin_port= ::htons( address.port );

	const int connection_result= ::connect( tcp_socket, (sockaddr*) &server_tcp_address, sizeof(server_tcp_address) );
	if( connection_result == SOCKET_ERROR )
	{
		Log::Warning( FUNC_NAME, " can not connect to server. Error code: ", ::WSAGetLastError() );
		::closesocket( tcp_socket );
		::closesocket( udp_socket );
		return nullptr;
	}

	// Recive protocol version.
	uint32_t protocol_version;
	::recv( tcp_socket, (char*) &protocol_version, sizeof(protocol_version), 0 ); // TODO - check errors.
	if( protocol_version != Messages::c_protocol_version )
	{
		Log::Warning( FUNC_NAME, "Can not connect to server - protocol version mismatch." );
		::closesocket( tcp_socket );
		::closesocket( udp_socket );
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
	for( unsigned int n= 0u; n < 4u; n++ )
	{
		Messages::DummyNetMessage first_message;
		::sendto( udp_socket, (char*) &first_message, sizeof(first_message), 0, (sockaddr*) &server_udp_address, sizeof(server_udp_address) );
	}
#else
	// Create and open TCP socket.
	const SOCKET tcp_socket= ::socket( AF_INET, SOCK_STREAM, 0 );
	if( tcp_socket == -1 )
	{
		Log::Warning( FUNC_NAME, " can not create tcp socket. Error code: ", errno );
		return nullptr;
	}

	sockaddr_in tcp_address;
	tcp_address.sin_family= AF_INET;
	tcp_address.sin_addr.s_addr= INADDR_ANY;
	tcp_address.sin_port= htons( in_tcp_port );
	if( ::bind( tcp_socket, (sockaddr*) &tcp_address, sizeof(tcp_address) ) != 0 )
	{
		Log::Warning( FUNC_NAME, " can not bind tcp socket. Error code: ", errno );
		::close( tcp_socket );
		return nullptr;
	}

	// Createe and open UDP socket.
	const SOCKET udp_socket= ::socket( AF_INET, SOCK_DGRAM, 0 );
	if( udp_socket == -1 )
	{
		Log::Warning( FUNC_NAME, " can not create udp socket. Error code: ", errno );
		::close( tcp_socket );
		return nullptr;
	}

	sockaddr_in udp_address;
	udp_address.sin_family= AF_INET;
	udp_address.sin_addr.s_addr= INADDR_ANY;
	udp_address.sin_port= htons( in_udp_port );
	if( ::bind( udp_socket, (sockaddr*) &udp_address, sizeof(udp_address) ) != 0 )
	{
		Log::Warning( FUNC_NAME, " can not bind udp socket. Error code: ", errno );
		::close( tcp_socket );
		::close( udp_socket );
		return nullptr;
	}

	// Connect to server.
	sockaddr_in server_tcp_address;
	std::memset( &server_tcp_address, 0, sizeof(server_tcp_address) );
	server_tcp_address.sin_family= AF_INET;
	server_tcp_address.sin_addr.s_addr= htonl( address.ip_address );
	server_tcp_address.sin_port= htons( address.port );

	const int connection_result= ::connect( tcp_socket, (sockaddr*) &server_tcp_address, sizeof(server_tcp_address) );
	if( connection_result == -1 )
	{
		Log::Warning( FUNC_NAME, " can not connect to server. Error code: ", errno );
		::close( tcp_socket );
		::close( udp_socket );
		return nullptr;
	}

	// Recive protocol version.
	uint32_t protocol_version;
	::recv( tcp_socket, (char*) &protocol_version, sizeof(protocol_version), 0 ); // TODO - check errors.
	if( protocol_version != Messages::c_protocol_version )
	{
		Log::Warning( FUNC_NAME, "Can not connect to server - protocol version mismatch." );
		::close( tcp_socket );
		::close( udp_socket );
		return nullptr;
	}

	// Recive from server it input udp address.
	uint16_t server_udp_port;
	::recv( tcp_socket, (char*) &server_udp_port, sizeof(server_udp_port), 0 ); // TODO - check errors.
	sockaddr_in server_udp_address;
	std::memcpy( &server_udp_address, &server_tcp_address, sizeof(sockaddr_in) );
	server_udp_address.sin_port= htons( server_udp_port );

	// Send to server first udp message for establishing of connection.
	// Make NAT happy.
	// Make this multiple times, for better reliability.
	for( unsigned int n= 0u; n < 4u; n++ )
	{
		Messages::DummyNetMessage first_message;
		::sendto( udp_socket, (char*) &first_message, sizeof(first_message), 0, (sockaddr*) &server_udp_address, sizeof(server_udp_address) );
	}
#endif

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
