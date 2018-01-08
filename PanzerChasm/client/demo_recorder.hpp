#pragma once
#include "messages_extractor.hpp"
#include "../Common/files.hpp"

namespace PanzerChasm
{

class DemoRecorder
{
public:
	DemoRecorder(
		const char* out_demo_file_name,
		MessagesExtractor& messages_extractor );
	~DemoRecorder();

	template<class MessagesHandler>
	void ProcessMessages( MessagesHandler& messages_handler );

private:
	template<class MessagesHandler>
	class InternalHandler;

	void BeginFrame();
	void EndFrame();

private:
	std::FILE* const file_;
	MessagesExtractor& messages_extractor_;
};

// Inline stuff. TODO - mowe it to separate file.

template<class MessagesHandler>
class DemoRecorder::InternalHandler final
{
public:
	InternalHandler( std::FILE* const file, MessagesHandler& external_messages_handler )
		: file_(file)
		, external_messages_handler_(external_messages_handler)
	{}

	template<class Message>
	void operator()( const Message& message )
	{
		external_messages_handler_(message);
		FileWrite( file_, &message, sizeof(Message) );
	}

private:
	std::FILE* const file_;
	MessagesHandler& external_messages_handler_;
};

template<class MessagesHandler>
void DemoRecorder::ProcessMessages( MessagesHandler& messages_handler )
{
	BeginFrame();

	InternalHandler<MessagesHandler> internal_handler( file_, messages_handler );
	messages_handler.ProcessMessages( internal_handler );

	EndFrame();
}

} // namespace PanzerChasm
