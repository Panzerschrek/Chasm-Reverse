#pragma once

namespace PanzerChasm
{

class HostCommands
{
public:
	virtual ~HostCommands(){}

	virtual void Quit()= 0;

	/*
	virtual void NewGame( unsigned int difficulty )= 0;
	virtual void SaveGame()= 0;
	virtual void LoadGame()= 0;
	*/
};

} // namespace PanzerChasm
