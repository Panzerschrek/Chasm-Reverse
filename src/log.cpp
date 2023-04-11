#include <SDL_messagebox.h>

#include "log.hpp"

namespace PanzerChasm
{

Log::LogCallback Log::log_callback_;
std::ofstream Log::log_file_{ "panzer_chasm.log" };

void Log::SetLogCallback( LogCallback callback )
{
	log_callback_= std::move(callback);
}

void Log::ShowFatalMessageBox( const std::string& error_message )
{
	SDL_ShowSimpleMessageBox(
		SDL_MESSAGEBOX_ERROR,
		"Fatal error",
		error_message.c_str(),
		nullptr );
}

} // namespace PanzerChasm
