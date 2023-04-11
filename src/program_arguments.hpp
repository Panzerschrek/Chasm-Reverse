#pragma once

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

private:
	unsigned int argc_;
	const char* const* const argv_;
};

} // namespace PanzerChasm
