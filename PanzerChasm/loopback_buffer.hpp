#pragma once

#include "server/i_connections_listener.hpp"

namespace PanzerChasm
{

class LoopbackBuffer final : public IConnectionsListener
{
public:
	LoopbackBuffer();
	virtual ~LoopbackBuffer() override;

public: // IConnectionsListener
	virtual IConnectionPtr GetNewConnection() override;

private:
};

} // namespace PanzerChasm
