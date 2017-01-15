#pragma once
#include "../fwd.hpp"

namespace PanzerChasm
{

struct InetAddress
{
	uint32_t address;
	uint16_t port;

	// Returns true, if successfully parsed.
	static bool Parse( const std::string& address_string, InetAddress& out_address );
};

class Net final
{
public:
	static constexpr uint16_t c_default_tcp_port= 6666u;
	static constexpr uint16_t c_default_udp_port= 8000u;

	Net();
	~Net();

	IConnectionPtr ConnectToServer( const InetAddress& address, uint16_t in_udp_port );
	IConnectionsListenerPtr CreateServerListener(
		uint16_t tcp_port= c_default_tcp_port,
		uint16_t base_udp_port= c_default_udp_port );

private:
	struct PlatformData;

private:
	std::unique_ptr<PlatformData> platform_data_;
	bool successfully_started_= false;
};

} // namespace PanzerChasm
