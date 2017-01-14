#pragma once
#include <vec.hpp>

#include "../client/map_state.hpp"
#include "../fwd.hpp"
#include "../time.hpp"

namespace PanzerChasm
{

namespace Sound
{

class ObjectsSoundsProcessor final
{
public:
	explicit ObjectsSoundsProcessor( const GameResourcesConstPtr& game_resources );
	~ObjectsSoundsProcessor();

	void SetMap( const MapDataConstPtr& map_data );
	void Update( const MapState& map_state, const m_Vec3& head_pos );

	unsigned int GetCurrentSoundNumber() const; // 0 - means no sound.
	const m_Vec3& GetCurrentSoundPosition() const; // invalid, if sound number is zero.
	float GetCurrentSoundVolume() const;

private:
	const GameResourcesConstPtr game_resources_;

	Time prev_tick_time_;

	MapDataConstPtr map_data_;

	m_Vec3 nearest_source_pos_;
	float volume_= 0.0f;
	unsigned int current_sound_= 0u;
};

} // namespace Sound

} // namespace PanzerChasm
