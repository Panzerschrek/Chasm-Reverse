#pragma once
#include <memory>

#include "system_event.hpp"
#include "system_window.hpp"

namespace PanzerChasm
{

class Host final
{
public:
	Host();
	~Host();

	// Returns false on quit
	bool Loop();

private:
	// Put members here in reverse deinitialization order.

	bool quit_requested_= false;

	std::unique_ptr<SystemWindow> system_window_;
	SystemEvents events_;
};

} // namespace PanzerChasm
