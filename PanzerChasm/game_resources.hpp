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
	constexpr static unsigned int c_max_file_name_size= 20u;
	constexpr static unsigned int c_max_file_path_size= 64u;

	struct ItemDescription
	{
		char model_file_name[ c_max_file_name_size ];
		char animation_file_name[ c_max_file_name_size ];

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
		char model_file_name[ c_max_file_name_size ];
		float w_radius, a_radius;
		int speed, r;
		int life;
		int kick; // Kick damage.
		int rock; // Rocket numer. -1 - no remote attack, maybe
		int sep_limit;
	};

	struct SpriteEffectDescription
	{
		char sprite_file_name[ c_max_file_name_size ];
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
		char model_file_name[ c_max_file_name_size ];
		char animation_file_name[ c_max_file_name_size ];
		char reloading_animation_file_name[ c_max_file_name_size ];

		int r_type; // type of projectile (bullet, grenade, etc. )
		int reloading_time;
		int y_sh; // unknown
		int r_z0; // vertical shift of shot point. bigger - lower
		int d_am; // ammo box size
		int limit; // ammo limit
		int start; // initial ammo count
		int r_count; // projectiles count ( 1 for rifle, many for shotgun )
	};

	struct RocketDescription
	{
		// If there is no model - shot is like "bullet":
		// gravity ignored, speed is infinite, no blast damage.
		char model_file_name[ c_max_file_name_size ];
		char animation_file_name[ c_max_file_name_size ];

		int blow_effect; // 1 - sparcles, 2 - blast, 4 - Mega-Destroyer blast.
		int gravity_force; // gravity. 0 - fly straight.
		int Ard; // Attack radius ( explosion damage distance )
		int CRd; // Unknown - always 42
		int power;

		bool reflect; // Reflect from floor/ceiling, but not from walls and models.
		bool fullbright;
		bool Light; // Shot emit light
		bool Auto; // Maybe, autoaim, but not working
		bool Auto2; // Autoaim. Rocket selects as target nearest player.
		bool fast; // Speed is greater, if nonzero
		int smoke_trail_effect_id; // Trail sprite effect type
	};

public:
	VfsPtr vfs;
	Palette palette; // main game palette

	std::vector<ItemDescription> items_description;
	std::vector<Model> items_models;

	std::vector<MonsterDescription> monsters_description;
	std::vector<Model> monsters_models;

	std::vector<SpriteEffectDescription> sprites_effects_description;
	std::vector<ObjSprite> effects_sprites;

	std::vector<WeaponDescription> weapons_description;
	std::vector<Model> weapons_models;

	std::vector<RocketDescription> rockets_description;
	std::vector<Model> rockets_models;
};

typedef std::shared_ptr<GameResources> GameResourcesPtr;
typedef std::shared_ptr<const GameResources> GameResourcesConstPtr;

GameResourcesConstPtr LoadGameResources( const VfsPtr& vfs );

} // namespace PanzerChasm
