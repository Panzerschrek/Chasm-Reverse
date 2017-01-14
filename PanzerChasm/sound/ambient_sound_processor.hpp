#pragma once
#include <vec.hpp>

#include "../fwd.hpp"
#include "../time.hpp"

namespace PanzerChasm
{

namespace Sound
{

class AmbientSoundProcessor final
{
public:
	AmbientSoundProcessor();
	~AmbientSoundProcessor();

	void SetMap( const MapDataConstPtr& map_data );

	void UpdatePosition( const m_Vec2& pos );

	unsigned int GetCurrentSoundNumber() const; // 0 - means no sound.
	float GetCurrentSoundVolume() const; // [ 0; 1 ]

private:
	MapDataConstPtr map_data_;

	Time prev_tick_time_;

	float volume_= 0.0f; // [ 0.0 ; 1.0 ]
	unsigned int current_sound_= 0u;
};

} // namespace Sound

} // namespace PanzerChasm
