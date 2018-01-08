#include "../log.hpp"
#include "demo_recorder.hpp"

namespace PanzerChasm
{

DemoRecorder::DemoRecorder(
	const char* const out_demo_file_name,
	MessagesExtractor& messages_extractor )
	: file_( std::fopen( out_demo_file_name, "wb" ) )
	, messages_extractor_(messages_extractor)
{
	if( file_ == nullptr )
		Log::Warning( "Can not open file for demo write: ", out_demo_file_name );

	// TODO - write demo header here
}

DemoRecorder::~DemoRecorder()
{
	if( file_ != nullptr )
		std::fclose( file_ );
}

void DemoRecorder::BeginFrame()
{
	// TODO
}

void DemoRecorder::EndFrame()
{
	if( file_ != nullptr )
		std::fflush( file_ );
}

} // namespace PanzerChasm
