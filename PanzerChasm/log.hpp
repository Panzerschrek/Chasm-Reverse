#pragma once
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>

namespace PanzerChasm
{

// Simple logger. You can write messages to it.
// Not thread-safe.
class Log
{
public:
	// Log-out function.
	typedef std::function<void(std::string)> LogCallback;

	static void SetLogCallback( LogCallback callback );

	template<class...Args>
	static void Info(const Args&... args );

	template<class...Args>
	static void Warning( const Args&... args );

	// Exits application.
	template<class...Args>
	static void FatalError( const Args&... args );

private:
	static inline void Print( std::ostringstream& ){}

	template<class Arg0, class... Args>
	static void Print( std::ostringstream& stream, const Arg0& arg0, const Args&... args );

	template<class... Args>
	static void PrinLine( const Args&... args );

	static void ShowFatalMessageBox( const std::string& error_message );

private:
	static LogCallback log_callback_;
	static std::ofstream log_file_;
};

template<class... Args>
void Log::Info(const Args&... args )
{
	PrinLine( args... );
}

template<class... Args>
void Log::Warning( const Args&... args )
{
	PrinLine( args... );
}

template<class... Args>
void Log::FatalError( const Args&... args )
{
	std::ostringstream stream;
	Print( stream, args... );
	const std::string str= stream.str();

	std::cout << str << std::endl;
	log_file_ << str << std::endl;
	ShowFatalMessageBox( str );

	std::exit(-1);
}

template<class Arg0, class... Args>
void Log::Print( std::ostringstream& stream, const Arg0& arg0, const Args&... args )
{
	stream << arg0;
	Print( stream, args... );
}

template<class... Args>
void Log::PrinLine( const Args&... args )
{
	std::ostringstream stream;
	Print( stream, args... );
	const std::string str= stream.str();

	std::cout << str << std::endl;
	log_file_ << str << std::endl;

	if( log_callback_ != nullptr )
		log_callback_( std::move(str) );
}

} // namespace PanzerChasm
