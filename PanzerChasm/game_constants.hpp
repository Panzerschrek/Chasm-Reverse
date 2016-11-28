#pragma once

namespace PanzerChasm
{

// Experimental game constants here.
namespace GameConstants
{

constexpr unsigned int weapon_count= 8u;

constexpr float player_eyes_level= 0.75f; // From ground
constexpr float player_height= 0.9f;
constexpr float player_radius= 70.0f / 256.0f;
constexpr float player_interact_radius= 100.0f / 256.0f;
constexpr float player_z_pull_distance= 1.0f / 4.0f;

constexpr float walls_height= 2.0f;

const float animations_frames_per_second= 20.0f;

// TODO - calibrate rockets parameters.
const float rockets_gravity_scale= 1.5f;
const float rockets_speed= 10.0f;
const float fast_rockets_speed= 20.0f;

} // namespace GameConstants

} // namespace PanzerChasm
