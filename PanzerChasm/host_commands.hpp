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
	/*
	virtual void SaveGame()= 0;
	virtual void LoadGame()= 0;
	*/
};

} // namespace PanzerChasm
