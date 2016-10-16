#include "host.hpp"

namespace PanzerChasm
{

Host::Host()
{
	system_window_.reset( new SystemWindow() );
}

Host::~Host()
{
}

bool Host::Loop()
{
	if( system_window_ )
		system_window_->GetInput( events_ );

	// TODO - send events to their destination
	for( const SystemEvent& event : events_ )
	{
		if( event.type == SystemEvent::Type::Quit )
			quit_requested_= true;
	}
	events_.clear();

	if( system_window_ )
		system_window_->SwapBuffers();

	return !quit_requested_;
}

} // namespace PanzerChasm
