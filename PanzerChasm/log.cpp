#include "log.hpp"

namespace PanzerChasm
{

Log::LogCallback Log::log_callback_;

void Log::SetLogCallback( LogCallback callback )
{
	log_callback_= std::move(callback);
}

} // namespace PanzerChasm
