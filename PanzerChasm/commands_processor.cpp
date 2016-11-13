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
	const std::pair< std::string, CommandsArguments > command_parsed= ParseCommad( command_string );
	if( command_parsed.first.empty() )
		return;

	for( const auto& arg : command_parsed.second )
		Log::Info( arg );

	for( unsigned int m= 0u; m < commands_maps_.size(); )
	{
		const CommandsMapConstPtr commads_map= commands_maps_[m].lock();
		if( commads_map != nullptr )
		{
			const auto it= commads_map->find( command_parsed.first );
			if( it != commads_map->end() )
			{
				// Found it
				it->second( command_parsed.second );
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

	Log::Info( command_parsed.first, ": command not found" );
}

std::pair< std::string, CommandsArguments > CommandsProcessor::ParseCommad(
	const char* const command_string )
{
	std::pair< std::string, CommandsArguments > result;

	const char* s= command_string;

	// Skip trailing spaces
	while( *s != '\0' && std::isspace( *s ) ) s++;

	// Process command name
	while( *s != '\0' && !std::isspace( *s ) )
	{
		result.first.push_back( std::tolower( *s ) );
		s++;
	}

	while( *s != '\0' )
	{
		// Skip trailing spaces
		while( *s != '\0' && std::isspace( *s ) ) s++;

		// Do not create argument if string is over.
		if( *s == '\0' ) break;

		result.second.emplace_back();

		// Process argument
		if( *s == '"' ) // Arg in quotes
		{
			s++;
			while( *s != '\0' && *s != '"' )
			{
				if( *s == '\\' && s[1] == '"' ) s++; // Escaped quote
				result.second.back().push_back( *s );
				s++;
			}
			if( *s == '"' )
				s++;
			else
				break;
		}
		else // Space-separated argument
		{
			while( *s != '\0' && !std::isspace( *s ) )
			{
				result.second.back().push_back( *s );
				s++;
			}
		}
	}

	return result;
}

} // namespace PanzerChasm
