// main.cpp - entry point

#include <memory>
#include <iostream>

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

	// Print exit text to console.
	// Authors list is reconstructed. In original game it was hardcoded in executable.
	//TODO - maybe make system window with this text?
	static const char c_exit_screen_text[]=
R"(
                                  PanzerChasm
                    (c) 2016-2023 Artjom "Panzerschrek" Kunz,
                              github contributors
--------------------------------------------------------------------------------
                                CHASM - The Rift
                      (C) Megamedia Corp, Action Forms Ltd.
                          Published by Megamedia Corp.
                         Developed by Action Forms Ltd.
--------------------------------------------------------------------------------
               Programming      :  Oleg Slusar
               Artwork          :  Yaroslav Kravchenko,
                                   Alexey Serhiy,
                                   Alexey Pivtorak
               SFX and Music    :  Alex Kot
               Level Design     :  Yaroslav Kravchenko, Denis Vereschagin
                                   Andrey Sharanevitch, Alexey Pechenkin
               Directed by      :  Igor Karev, Denis Vereschagin
               Special thanks to:  Alexander Soroka, Peter M.Kolos
--------------------------------------------------------------------------------
)";

	std::cout << c_exit_screen_text;
	return 0;
}
