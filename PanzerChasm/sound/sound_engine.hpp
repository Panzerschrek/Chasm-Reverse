#pragma once
#include <vec.hpp>

#include "../fwd.hpp"
#include "../game_resources.hpp"
#include "../map_loader.hpp"
#include "channel.hpp"
#include "driver.hpp"
#include "sounds_loader.hpp"

namespace PanzerChasm
{

namespace Sound
{

class SoundEngine final
{
public:
	explicit SoundEngine( const GameResourcesConstPtr& game_resources );
	~SoundEngine();

	void Tick();

	void SetMap( const MapDataConstPtr& map_data );

	void SetHeadPosition(
		const m_Vec3& position,
		float z_angle, float x_angle );

	void PlayWorldSound(
		unsigned int sound_number,
		const m_Vec3& position );

	void PlayHeadSound( unsigned int sound_number );

private:
	struct Source
	{
		bool is_free= true;

		unsigned int sound_id;
		unsigned int pos_samples;
		bool is_head_relative;
		m_Vec3 pos;

		float volume[2]; // calculated each tick
	};

private:
	Source* GetFreeSource();
	void CalculateSourcesVolume();
	void ForceStopAllChannels();

private:
	// TODO - check this numbers
	static constexpr unsigned int c_max_monsters= 24u;
	static constexpr unsigned int c_max_monster_sounds= 10u;
	static constexpr unsigned int c_max_total_monsters_sounds= c_max_monsters * c_max_monster_sounds;

private:
	const GameResourcesConstPtr game_resources_;

	Driver driver_;

	MapDataConstPtr current_map_data_;

	std::array<
		ISoundDataConstPtr,
		GameResources::c_max_global_sounds + MapData::c_max_map_sounds + c_max_total_monsters_sounds >
		sounds_;

	Source sources_[ Channel::c_max_channels ];

	m_Vec3 head_position_;
	m_Vec3 ears_vectors_[2];
};

} // namespace Sound

} // namespace PanzerChasm
