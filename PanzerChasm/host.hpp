#pragma once
#include <memory>

#include "client/client.hpp"
#include "commands_processor.hpp"
#include "console.hpp"
#include "host_commands.hpp"
#include "menu.hpp"
#include "server/server.hpp"
#include "system_event.hpp"
#include "system_window.hpp"

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
	void NewGame();
	void RunLevel( const CommandsArguments& args );

private:
	// Put members here in reverse deinitialization order.

	bool quit_requested_= false;

	CommandsProcessor commands_processor_;
	CommandsMapConstPtr host_commands_;

	VfsPtr vfs_;
	GameResourcesConstPtr game_resources_;

	std::unique_ptr<SystemWindow> system_window_;
	SystemEvents events_;

	std::unique_ptr<Menu> menu_;
	std::unique_ptr<Console> console_;

	MapLoaderPtr map_loader_;

	LoopbackBufferPtr loopback_buffer_;
	std::unique_ptr<Server> local_server_;

	std::unique_ptr<Client> client_;
};

} // namespace PanzerChasm
