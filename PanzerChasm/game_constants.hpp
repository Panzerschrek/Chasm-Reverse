#pragma once

namespace PanzerChasm
{

// Experimental game constants here.
namespace GameConstants
{

// Screen sizeo fo original game.
constexpr unsigned int min_screen_width = 320u;
constexpr unsigned int min_screen_height= 200u;

constexpr unsigned int weapon_count= 8u;
constexpr unsigned int mine_weapon_number= 6u;
constexpr unsigned int mega_destroyer_weapon_number= 7u;

constexpr unsigned int backpack_item_id= 0u;
constexpr unsigned int mine_item_id= 30u;
constexpr unsigned int shield_item_id= 21u;

// TODO - calibrate mines parameters.
constexpr float mines_activation_radius= 0.5f;
constexpr float mines_explosion_radius= 2.3f;
constexpr int mines_damage= 140;
constexpr float mines_preparation_time_s= 1.5f;

constexpr int player_nominal_health= 100;
constexpr int player_max_health= 200;
constexpr int player_max_armor= 200;

constexpr float player_eyes_level= 0.75f; // From ground
constexpr float player_deathcam_level= 1.4f;
constexpr float player_height= 0.9f;
constexpr float player_radius= 60.0f / 256.0f;
constexpr float player_interact_radius= 100.0f / 256.0f;
constexpr float z_pull_distance= 1.0f / 2.5f;
constexpr float z_pull_speed= 2.5f;

constexpr float invisibility_time_s= 40.0f; // Time of full visibility.
constexpr float invisibility_flashing_start_time_s= 30.0f;

constexpr float shield_time_s= 40.0f;
constexpr float shield_flashing_start_time_s= 30.0f;

// Ticks in second, when monsters recieve damage from death zones.
constexpr float death_ticks_per_second= 3.0f;
constexpr float mortal_walls_damage_per_second= 750.0f;

const float player_max_speed= 5.0f; // Max speed, which player can reach himself.
const float player_max_absolute_speed= 15.0f; // Maximum speed of player.

constexpr float walls_height= 2.0f;

constexpr float procedures_speed_scale= 1.0f / 10.0f;

const float animations_frames_per_second= 20.0f;
const float weapons_animations_frames_per_second= 30.0f;
const float sprites_animations_frames_per_second= 15.0f;

const float weapon_switch_time_s= 0.7f;

// TODO - calibrate rockets parameters.
const float rockets_gravity_scale= 1.5f;
const float rockets_speed= 10.0f;
const float fast_rockets_speed= 20.0f;

constexpr float vertical_acceleration= -9.8f;
constexpr float max_vertical_speed= 5.0f;

constexpr float particles_vertical_acceleration= -5.0f;

const unsigned int player_colors_count= 11u;
const int player_colors_shifts[ player_colors_count ]=
{
	0, // Original blue
	- 2 * 16,
	- 5 * 16, // green.
	- 9 * 16,
	-12 * 16,
	- 6 * 16,
	-14 * 16,
	- 1 * 16,
	// Additional colors in PanzerChasm.
	-10 * 16,
	- 7 * 16,
	- 8 * 16
};

const unsigned int max_players= 8u;

} // namespace GameConstants

} // namespace PanzerChasm
