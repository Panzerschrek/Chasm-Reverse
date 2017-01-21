#pragma once

#include <fwd.hpp>

namespace PanzerChasm
{

class HostCommands
{
public:
	virtual ~HostCommands(){}

	virtual void Quit()= 0;


	virtual void NewGame( DifficultyType difficulty )= 0;

	virtual void ConnectToServer(
		const char* server_address, // 127.0.0.1:1488 for example
		uint16_t client_tcp_port,
		uint16_t client_udp_port )= 0;

	virtual void StartServer(
		unsigned int map_number,
		DifficultyType difficulty,
		bool dedicated,
		uint16_t server_tcp_port,
		uint16_t server_base_udp_port )= 0;
	/*
	virtual void SaveGame()= 0;
	virtual void LoadGame()= 0;
	*/
};

} // namespace PanzerChasm
