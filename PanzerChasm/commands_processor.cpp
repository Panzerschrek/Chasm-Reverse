#include <algorithm>
#include <cstring>

#include "log.hpp"
#include "settings.hpp"

#include "commands_processor.hpp"

namespace PanzerChasm
{

CommandsProcessor::CommandsProcessor( Settings& settings )
	: settings_(settings)
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

	// Search settins variable.
	if( settings_.IsValue( command_parsed.first.c_str() ) )
	{
		if( command_parsed.second.empty() )
			Log::Info( "\"", command_parsed.first, "\" is \"", settings_.GetString( command_parsed.first.c_str() ), "\"" );
		else
			settings_.SetSetting( command_parsed.first.c_str(), command_parsed.second.front().c_str() );
	}
	else
		Log::Info( command_parsed.first, ": command not found" );
}

std::string CommandsProcessor::TryCompleteCommand( const char* command_string ) const
{
	const std::string command= ParseCommad( command_string ).first;

	std::vector<std::string> candidates;

	for( unsigned int m= 0u; m < commands_maps_.size(); m++ )
	{
		const CommandsMapConstPtr commads_map= commands_maps_[m].lock();
		if( commads_map != nullptr )
		{
			for( const CommandsMap::value_type& command_value : *commads_map )
			{
				const char* const str= command_value.first.c_str();
				if( std::strstr( str, command.c_str() ) == str )
					candidates.push_back( command_value.first );
			}
		}
	}

	if( candidates.size() == 0u )
		return command;

	// Print candidates.
	if( candidates.size() > 1u )
	{
		Log::Info( ">", command );
		std::sort( candidates.begin(), candidates.end() );
		for( const std::string& candidate : candidates )
			Log::Info( "  ", candidate );
	}

	// Find common prefix for candidates.
	unsigned int pos= command.size();
	while( 1u )
	{
		if( pos >= candidates[0].size() )
			break;

		bool equals= true;
		for( const std::string& candidate : candidates )
			if( pos >= candidate.size() || candidate[pos] != candidates[0][pos] )
			{
				equals= false;
				break;
			}

		if( !equals )
			break;
		pos++;
	}

	return std::string( candidates[0].begin(), candidates[0].begin() + pos );
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
