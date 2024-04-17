#pragma once

#include <fwd.hpp>

namespace PanzerChasm
{

class HostCommands
{
public:
	virtual ~HostCommands(){}

	virtual Settings& GetSettings()= 0;
	virtual const SystemWindow* GetSystemWindow()= 0;

	virtual MapLoaderPtr GetMapLoader()= 0;
	virtual MapDataConstPtr CurrentMap() = 0;

	virtual void Quit()= 0;
	virtual void NewGame( DifficultyType difficulty )= 0;

	virtual void ConnectToServer(
		const char* server_address, // 127.0.0.1:1488 for example
		uint16_t client_tcp_port,
		uint16_t client_udp_port )= 0;

	virtual void StartServer(
		unsigned int map_number,
		DifficultyType difficulty,
		GameRules game_rules,
		bool dedicated,
		uint16_t server_tcp_port,
		uint16_t server_base_udp_port )= 0;

	static constexpr unsigned int c_save_slots= 10u;
	typedef std::array< SaveComment, c_save_slots > SavesNames;

	virtual void GetSavesNames( SavesNames& out_saves_names )= 0;

	virtual bool SaveAvailable() const = 0;
	virtual void SaveGame( uint8_t slot_number )= 0;
	virtual void LoadGame( uint8_t slot_number )= 0;

	virtual void VidRestart()= 0;
};

} // namespace PanzerChasm
