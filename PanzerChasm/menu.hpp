#pragma once
#include <memory>

#include "fwd.hpp"
#include "host_commands.hpp"
#include "rendering_context.hpp"
#include "system_event.hpp"

namespace PanzerChasm
{

class MenuBase;
class MainMenu;

// Main game munu class.
class Menu final
{
public:
	Menu(
		HostCommands& host_commands,
		const SharedDrawersPtr& shared_drawers,
		const Sound::SoundEnginePtr& sound_engine );
	~Menu();

	bool IsActive() const;
	void Deactivate();

	void ProcessEvents( const SystemEvents& events );
	void ProcessEventsWhileNonactive( const SystemEvents& events );
	void Draw();

private:
	HostCommands& host_commands_;
	const SharedDrawersPtr shared_drawers_;

	std::unique_ptr<MainMenu> root_menu_;
	MenuBase* current_menu_= nullptr;
};

} // namespace PanzerChasm
