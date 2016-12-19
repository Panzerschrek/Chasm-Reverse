#pragma once
#include <vec.hpp>

#include "../fwd.hpp"

namespace PanzerChasm
{

namespace Sound
{

class SoundEngine final
{
public:
	explicit SoundEngine( const GameResourcesConstPtr& game_resources );
	~SoundEngine();

	void SetMap( const MapDataPtr& map_data );

	void SetHeadPosition(
		const m_Vec3& position,
		float z_angle, float x_angle );

	void PlayWorldSound(
		unsigned int sound_number,
		const m_Vec3& position );

	void PlayHeadSound( unsigned int sound_number );

private:
	MapDataPtr current_map_data_;
};

} // namespace Sound

} // namespace PanzerChasm
