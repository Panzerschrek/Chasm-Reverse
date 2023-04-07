#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "fwd.hpp"

namespace PanzerChasm
{

typedef std::vector<std::string> CommandsArguments;

typedef std::function<void( const CommandsArguments&)> CommandFunction;
typedef std::unordered_map< std::string, CommandFunction > CommandsMap;
typedef std::shared_ptr<CommandsMap> CommandsMapPtr;
typedef std::shared_ptr<const CommandsMap> CommandsMapConstPtr;
typedef std::weak_ptr<const CommandsMap> CommandsMapConstWeakPtr;

class CommandsProcessor final
{
public:
	explicit CommandsProcessor( Settings& settings );
	~CommandsProcessor();

	// Register commands.
	// Commands owner must store pointer to commands map.
	void RegisterCommands( CommandsMapConstWeakPtr commands );

	void ProcessCommand( const char* command_string );

	// Retruns completed string, prints to log candidates commands.
	std::string TryCompleteCommand( const char* command_string ) const;

private:
	// Extract command from string, convert commnad to lower case, extract arguments
	static std::pair< std::string, CommandsArguments > ParseCommad( const char* command_string );

private:
	std::vector< CommandsMapConstWeakPtr > commands_maps_;
	Settings& settings_;

};

} // namespace PanzerChasm
