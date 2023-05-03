#include "program_arguments.hpp"

namespace PanzerChasm
{

ProgramArguments::ProgramArguments( const int argc, const char* const* const argv )
	: argc_( static_cast<unsigned int>(argc) )
	, argv_( argv )
{}

ProgramArguments::~ProgramArguments()
{}

bool ProgramArguments::HasParam( const char* const param_name ) const
{
	for( unsigned int i= 0u; i < argc_; i++ )
	{
		const char* const arg= argv_[i];
		if( arg[0] == '-' && arg[1] == '-' )
		{
			if( std::strcmp( param_name, arg + 2u ) == 0 )
				return true;
		}
	}

	return false;
}

const char* ProgramArguments::GetParamValue( const char* const param_name ) const
{
	for( unsigned int i= 0u; i < argc_; i++ )
	{
		const char* const arg= argv_[i];
		if( arg[0] == '-' && arg[1] == '-' )
		{
			if( std::strcmp( param_name, arg + 2u ) == 0 )
			{
				if( i + 1 < argc_ )
					return argv_[ i + 1u ];
				else
					return nullptr;
			}
		}
	}

	return nullptr;
}

} // namespace PanzerChasm
