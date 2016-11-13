#include "log.hpp"

#include "commands_processor.hpp"

namespace PanzerChasm
{

CommandsProcessor::CommandsProcessor()
{}

CommandsProcessor::~CommandsProcessor()
{}

void CommandsProcessor::RegisterCommands( CommandsMapConstWeakPtr commands )
{
	commands_maps_.emplace_back( std::move( commands ) );
}

void CommandsProcessor::ProcessCommand( const char* const command_string )
{
	// TODO - prepare command string: remove trailing spaces, parse arguments, convert to lower case, etc.

	for( unsigned int m= 0u; m < commands_maps_.size(); )
	{
		const CommandsMapConstPtr commads_map= commands_maps_[m].lock();
		if( commads_map != nullptr )
		{
			const auto it= commads_map->find( command_string );
			if( it != commads_map->end() )
			{
				// Found it
				it->second();
				return;
			}

			m++;
		}
		else
		{
			// Remove expired weak pointers.
			if( m != commands_maps_.size() - 1u )
				commands_maps_[m]= commands_maps_.back();
			commands_maps_.pop_back();
		}
	}

	Log::Info( command_string, ": command not found" );
}

} // namespace PanzerChasm
