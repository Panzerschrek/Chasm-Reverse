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
	Net();
	~Net();

	IConnectionPtr ConnectToServer( const InetAddress& address );
	IConnectionsListenerPtr CreateServerListener();

private:
	struct PlatformData;

private:
	std::unique_ptr<PlatformData> platform_data_;
	bool successfully_started_= false;
};

} // namespace PanzerChasm
