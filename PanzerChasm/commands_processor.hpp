#pragma once

namespace PanzerChasm
{

class CommandsProcessor final
{
public:
	CommandsProcessor();
	~CommandsProcessor();

	void ProcessCommand( const char* command_string );

private:
};

} // namespace PanzerChasm
