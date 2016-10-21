#pragma once
#include <memory>

#include "game_resources.hpp"
#include "host_commands.hpp"
#include "menu.hpp"
#include "system_event.hpp"
#include "system_window.hpp"
#include "vfs.hpp"

namespace PanzerChasm
{

class Host final : public HostCommands
{
public:
	Host();
	virtual ~Host() override;

	// Returns false on quit
	bool Loop();

public: // HostCommands
	virtual void Quit() override;

private:
	// Put members here in reverse deinitialization order.

	bool quit_requested_= false;

	VfsPtr vfs_;
	GameResourcesPtr game_resources_;

	std::unique_ptr<SystemWindow> system_window_;
	SystemEvents events_;

	std::unique_ptr<Menu> menu_;
};

} // namespace PanzerChasm
