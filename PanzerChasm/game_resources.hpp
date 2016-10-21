#pragma once

#include "images.hpp"
#include "vfs.hpp"


namespace PanzerChasm
{

// Commnon resources for different subsystems of client/server.
struct GameResources
{
	VfsPtr vfs;
	Palette palette; // main game palette

	// TODO - put monsters, weapoons, items description here
};

typedef std::shared_ptr<GameResources> GameResourcesPtr;

} // namespace PanzerChasm
