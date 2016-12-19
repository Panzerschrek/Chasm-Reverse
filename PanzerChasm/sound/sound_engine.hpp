#pragma once
#include <vec.hpp>

#include "../fwd.hpp"
#include "../game_resources.hpp"
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

	void SetMap( const MapDataPtr& map_data );

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
	};

private:
	const GameResourcesConstPtr game_resources_;

	Driver driver_;

	MapDataPtr current_map_data_;

	ISoundDataConstPtr global_sounds_[ GameResources::c_max_global_sounds ];

	Source sources_[ Channel::c_max_channels ];
};

} // namespace Sound

} // namespace PanzerChasm
