#pragma once

#include "fwd.hpp"
#include "images.hpp"
#include "model.hpp"
#include "obj.hpp"

namespace PanzerChasm
{

// Commnon resources for different subsystems of client/server.
struct GameResources
{
public:
	constexpr static unsigned int c_max_file_name_size= 20u;
	constexpr static unsigned int c_max_file_path_size= 64u;

	constexpr static unsigned int c_max_global_sounds= 80u;

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
		float w_radius; // Collisions radius ( with walls ).
		float attack_radius; // Also collision with player radius.
		int speed, rotation_speed;
		int life;
		int kick; // Kick damage.
		int rock; // Rocket numer. -1 - no remote attack, maybe
		int sep_limit; // Chance of body parts separation, percents.
	};

	struct SpriteEffectDescription
	{
		char sprite_file_name[ c_max_file_name_size ];

		// Something, like alpha
		// 0 - opaque
		// 1 - semi-transparent ~ 60%
		// 2 - semi-transparent ~ 30%
		// other values - unknown. Maybe transparency can chenge durign sprite animation?
		int glass;
		bool half_size;
		// Particle left small smoke trail
		bool smooking;
		bool looped;
		bool gravity;
		// particle reflects from floors (maybee ceilings too)
		bool jump;
		bool light_on;
	};

	struct BMPObjectDescription
	{
		char sprite_file_name[ c_max_file_name_size ];
		bool light;
		bool glass;
		bool half_size;
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

		/* For rockets:
		 * 1 - sparcles
		 * 2 - blast
		 * 3 - sparkles
		 * 4 - Mega-Destroyer blast
		 * other - sparcles
		 *
		 * For bullets:
		 * 1 - bullet hit
		 * 2, 3, 4 - no effect
		 * other - no effect
		*/
		int blow_effect;

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

	struct SoundDescription
	{
		static constexpr unsigned char c_max_volume= 128u;

		char file_name[ c_max_file_path_size ];
		unsigned char volume;
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

	std::vector<BMPObjectDescription> bmp_objects_description;
	std::vector<ObjSprite> bmp_objects_sprites;

	std::vector<WeaponDescription> weapons_description;
	std::vector<Model> weapons_models;

	std::vector<RocketDescription> rockets_description;
	std::vector<Model> rockets_models;

	SoundDescription sounds[ c_max_global_sounds ];
};

GameResourcesConstPtr LoadGameResources( const VfsPtr& vfs );

void LoadSoundsDescriptionFromMapResourcesFile(
	const Vfs::FileContent& resoure_file,
	GameResources::SoundDescription* out_sounds,
	unsigned int max_sound_count );

void LoadAmbientSoundsDescriptionFromMapResourcesFile(
	const Vfs::FileContent& resoure_file,
	GameResources::SoundDescription* out_sounds,
	unsigned int max_sound_count );

} // namespace PanzerChasm
