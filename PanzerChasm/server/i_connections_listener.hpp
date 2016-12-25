#pragma once

#include "../fwd.hpp"

namespace PanzerChasm
{

class IConnectionsListener
{
public:
	virtual ~IConnectionsListener(){}

	// Returns nullptr, if there is no new connections.
	// If there are many connections, this mehon need to call multiple times.
	virtual IConnectionPtr GetNewConnection()= 0;
};

typedef std::shared_ptr<IConnectionsListener> IConnectionsListenerPtr;

} // namespace PanzerChasm
