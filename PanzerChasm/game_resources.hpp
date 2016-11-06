#pragma once

#include "images.hpp"
#include "model.hpp"
#include "obj.hpp"
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

	struct SpriteEffectDescription
	{
		char sprite_file_name[16];
		int glass;
		bool half_size;
		bool smooking;
		bool looped;
		bool gravity;
		bool jump;
		bool light_on;
	};

	struct WeaponDescription
	{
		char model_file_name[16];
		char animation_file_name[16];
		char reloading_animation_file_name[16];

		int r_type; // type of projectile (bullet, grenade, etc. )
		int reloading_time;
		int y_sh; // unknown
		int r_z0; // vertical shift of shot point. bigger - lower
		int d_am; // ammo box size
		int limit; // ammo limit
		int start; // initial ammo count
		int r_count; // projectiles count ( 1 for rifle, many for shotgun )
	};

public:
	VfsPtr vfs;
	Palette palette; // main game palette

	std::vector<ItemDescription> items_description;
	std::vector<MonsterDescription> monsters_description;

	std::vector<Model> items_models;

	std::vector<SpriteEffectDescription> sprites_effects_description;
	std::vector<ObjSprite> effects_sprites;

	std::vector<WeaponDescription> weapons_description;
	std::vector<Model> weapons_models;
};

typedef std::shared_ptr<GameResources> GameResourcesPtr;
typedef std::shared_ptr<const GameResources> GameResourcesConstPtr;

GameResourcesConstPtr LoadGameResources( const VfsPtr& vfs );

} // namespace PanzerChasm
