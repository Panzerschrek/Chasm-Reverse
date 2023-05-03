#pragma once
#include <cstring>

namespace PanzerChasm
{

// Simple helper class for retireving command line arguments.
// Warning! argv must live longer, than "ProgramArguments" class.
class ProgramArguments final
{
public:
	ProgramArguments( int argc, const char* const* argv );
	~ProgramArguments();

	// Params names should start with "--" in command line, but here, "--" not needed.
	bool HasParam( const char* param_name ) const;
	const char* GetParamValue( const char* param_name ) const;

	template<typename Func>
	void EnumerateAllParamValues( const char* param_name, const Func& func ) const
	{
		for( unsigned int i= 0u; i < argc_; i++ )
		{
			const char* const arg= argv_[i];
			if( arg[0] == '-' && arg[1] == '-' && std::strcmp( param_name, arg + 2u ) == 0 && i + 1 < argc_ )
				func( argv_[ i + 1u ] );
		}
	}

private:
	unsigned int argc_;
	const char* const* const argv_;
};

} // namespace PanzerChasm
