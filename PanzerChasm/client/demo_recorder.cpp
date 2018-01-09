#include <cstring>
#include "../Common/files.hpp"
#include "../log.hpp"
#include "demo_recorder.hpp"

using namespace ChasmReverse;

namespace PanzerChasm
{

static const char g_expected_demo_header_id[9u]= "PanChDem"; // PanzerChasmDemo
static const char g_expected_demo_version= 1u;
static const MessageId frame_footer_message_id= static_cast<MessageId>( static_cast<unsigned int>(MessageId::NumMessages) + 1u );

struct DemoHeader
{
	char id[8];
	unsigned int demo_version;
	unsigned int protocol_version;
};

struct FrameHeader
{
	int64_t relative_time; // Un time units, see Time::GetInternalRepresentation
};

// Special message for end of demo frame.
struct FrameFooter : public Messages::MessageBase
{
	FrameFooter()
	{
		message_id= frame_footer_message_id;
	}
};

DemoRecorder::DemoRecorder(
	const char* const out_demo_file_name,
	MessagesExtractor& messages_extractor )
	: file_( std::fopen( out_demo_file_name, "wb" ) )
	, messages_extractor_(messages_extractor)
	, start_time_( Time::CurrentTime() )
{
	if( file_ == nullptr )
	{
		Log::Warning( "Can not open file for demo write: ", out_demo_file_name );
		return;
	}

	DemoHeader header;
	std::memcpy( header.id, g_expected_demo_header_id, sizeof(header.id) );
	header.demo_version= g_expected_demo_version;
	header.protocol_version= Messages::c_protocol_version;

	FileWrite( file_, &header, sizeof(DemoHeader) );
}

DemoRecorder::~DemoRecorder()
{
	if( file_ != nullptr )
		std::fclose( file_ );
}

void DemoRecorder::BeginFrame()
{
	FrameHeader header;
	header.relative_time= ( Time::CurrentTime() - start_time_ ).GetInternalRepresentation();
	FileWrite( file_, &header, sizeof(FrameHeader) );
}

void DemoRecorder::EndFrame()
{
	if( file_ == nullptr )
		return;

	FrameFooter footer;
	FileWrite( file_, &footer, sizeof(FrameFooter) );

	std::fflush( file_ );
}

} // namespace PanzerChasm
