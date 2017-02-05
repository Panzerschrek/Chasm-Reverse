// main.cpp - entry point

#include <memory>

#include <SDL.h>

#include "host.hpp"
using namespace PanzerChasm;

extern "C" int main( int argc, char *argv[] )
{
	// Skip first param - program path.
	argc--;
	argv++;

	// "Host" may be hard object. Create it on the heap.
	std::unique_ptr<Host> host( new Host( argc, argv ) );

	while( host->Loop() )
	{
	}

	return 0;
}
