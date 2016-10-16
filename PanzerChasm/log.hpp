#pragma once
#include <cstdlib>
#include <iostream>

namespace PanzerChasm
{

// Simple logger. You can write messages to it.
// Thread-safe.
class Log
{
public:
	template<class...Args>
	static void Info(const Args&... args );

	template<class...Args>
	static void Warning( const Args&... args );

	// Exits application.
	template<class...Args>
	static void FatalError( const Args&... args );

private:
	static inline void Print(){}

	template<class Arg0, class... Args>
	static void Print( const Arg0& arg0, const Args&... args );

	template<class... Args>
	static void PrinLine( const Args&... args );
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
	PrinLine( args... );
	std::exit( -1 );
}

template<class Arg0, class... Args>
void Log::Print( const Arg0& arg0, const Args&... args )
{
	std::cout << arg0;
	Print( args... );
}

template<class... Args>
void Log::PrinLine( const Args&... args )
{
	Print( args... );
	std::cout << std::endl;
}

} // namespace PanzerChasm
