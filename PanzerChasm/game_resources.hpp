#pragma once

#include "images.hpp"
#include "model.hpp"
#include "vfs.hpp"

namespace PanzerChasm
{

// Commnon resources for different subsystems of client/server.
struct GameResources
{
public:
	struct ItemDescription
	{
		char model_file_name[16];
		char animation_file_name[16];

		float radius;
		bool cast_shadow;
		int bmp_obj; // unknown
		int bmp_z; // unknown
		int a_code;
		int blow_up;
		int b_limit; // break limit?

		unsigned int sfx, b_sfx; // sounds or zero
	};

	struct MonsterDescription
	{
		char model_file_name[16];
		float w_radius, a_radius;
		int speed, r;
		int life;
		int kick;
		int rock;
		int sep_limit;
	};

public:
	VfsPtr vfs;
	Palette palette; // main game palette

	std::vector<ItemDescription> items_description;
	std::vector<MonsterDescription> monsters_description;

	std::vector<Model> items_models;
};

typedef std::shared_ptr<GameResources> GameResourcesPtr;
typedef std::shared_ptr<const GameResources> GameResourcesConstPtr;

GameResourcesConstPtr LoadGameResources( const VfsPtr& vfs );

} // namespace PanzerChasm
