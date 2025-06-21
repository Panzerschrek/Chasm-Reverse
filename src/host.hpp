#pragma once
#include <memory>

#include "client/client.hpp"
#include "commands_processor.hpp"
#include "console.hpp"
#include "host_commands.hpp"
#include "menu.hpp"
#include "net/net.hpp"
#include "program_arguments.hpp"
#include "server/server.hpp"
#include "settings.hpp"
#include "system_event.hpp"
#include "system_window.hpp"
#include "ticks_counter.hpp"

namespace PanzerChasm
{

class Host final : public HostCommands
{
public:
	Host( int argc, const char* const* argv );
	virtual ~Host() override;

	// Returns false on quit
	bool Loop();

public: // HostCommands
	virtual Settings& GetSettings() override;
	virtual const SystemWindow* GetSystemWindow() override;
	virtual MapLoaderPtr GetMapLoader() override;
	virtual MapDataConstPtr CurrentMap() override;

	virtual void Quit() override;
	virtual void NewGame( DifficultyType difficulty ) override;

	virtual void ConnectToServer(
		const char* server_address,
		uint16_t client_tcp_port,
		uint16_t client_udp_port ) override;

	virtual void StartServer(
		unsigned int map_number,
		DifficultyType difficulty,
		GameRules game_rules,
		bool dedicated,
		uint16_t server_tcp_port,
		uint16_t server_base_udp_port ) override;

	virtual void GetSavesNames( SavesNames& out_saves_names ) override;
	virtual bool SaveAvailable() const override;
	virtual void SaveGame( uint8_t slot_number ) override;
	virtual void LoadGame( uint8_t slot_number ) override;

	virtual void VidRestart() override;

private:
	class ConnectionsListenerProxy;

private:
	void NewGameCommand( const CommandsArguments& args );
	void RunLevelCommand( const CommandsArguments& args );
	void ConnectCommand( const CommandsArguments& args );
	void DisconnectCommand();
	void RunServerCommand( const CommandsArguments& args );
	void SaveCommand( const CommandsArguments& args );
	void LoadCommand( const CommandsArguments& args );

	void DoVidRestart();

	void DoRunLevel( unsigned int map_number, DifficultyType difficulty );
	void DoSave( const std::filesystem::path& save_file_name );
	void DoLoad( const std::filesystem::path& save_file_name );

	void DrawLoadingFrame( float progress, const char* caption );

	void EnsureClient();
	void EnsureServer();
	void EnsureLoopbackBuffer();
	void EnsureNet();

	void ClearBeforeGameStart();

private:
	// Put members here in reverse deinitialization order.

	bool quit_requested_= false;

	const ProgramArguments program_arguments_;
	Settings settings_;
	CommandsProcessor commands_processor_;
	CommandsMapConstPtr host_commands_;

	TicksCounter loops_counter_;

	VfsPtr vfs_;
	GameResourcesConstPtr game_resources_;

	std::unique_ptr<Net> net_;

	std::unique_ptr<SystemWindow> system_window_;
	SystemEvents events_;

	Sound::SoundEnginePtr sound_engine_;

	IDrawersFactoryPtr drawers_factory_;
	std::shared_ptr<SharedDrawers> shared_drawers_;
	std::unique_ptr<Console> console_;
	std::unique_ptr<Menu> menu_;

	MapLoaderPtr map_loader_;

	LoopbackBufferPtr loopback_buffer_;
	std::shared_ptr<ConnectionsListenerProxy> connections_listener_proxy_; // Create it together with server.
	std::unique_ptr<Server> local_server_;
	std::unique_ptr<Client> client_;

	std::string base_window_title_;
	bool is_single_player_= false;
	bool paused_= false;
};

} // namespace PanzerChasm
