#include "../map_loader.hpp"

#include "ambient_sound_processor.hpp"

namespace PanzerChasm
{

namespace Sound
{

AmbientSoundProcessor::AmbientSoundProcessor()
	: prev_tick_time_( Time::CurrentTime() )
{}

AmbientSoundProcessor::~AmbientSoundProcessor()
{}

void AmbientSoundProcessor::SetMap( const MapDataConstPtr& map_data )
{
	map_data_= map_data;

	current_sound_= 0u;
	volume_= 0u;
}

void AmbientSoundProcessor::UpdatePosition( const m_Vec2& pos )
{
	const float c_change_time_s= 0.5f;

	const Time current_tick_time= Time::CurrentTime();
	const float tick_delta_s= ( current_tick_time - prev_tick_time_ ).ToSeconds();
	prev_tick_time_= current_tick_time;

	unsigned int next_sound_id= 0u;

	if( map_data_ != nullptr )
	{
		unsigned int x= static_cast<unsigned int>( std::floor( pos.x ) );
		unsigned int y= static_cast<unsigned int>( std::floor( pos.y ) );
		if( x < MapData::c_map_size && y < MapData::c_map_size )
			next_sound_id= map_data_->ambient_sounds_map[ x + y * MapData::c_map_size ];
	}

	const float abs_volume_change= tick_delta_s / c_change_time_s;
	if( next_sound_id != current_sound_ )
	{
		volume_-= abs_volume_change;
		if( volume_ <= 0.0f )
		{
			current_sound_= next_sound_id;
			volume_= 0.0f;
		}
	}
	else
	{
		volume_+= abs_volume_change;
		if( volume_ >= 1.0f )
		{
			volume_= 1.0f;
		}
	}
}

unsigned int AmbientSoundProcessor::GetCurrentSoundNumber() const
{
	return current_sound_;
}

float AmbientSoundProcessor::GetCurrentSoundVolume() const
{
	return volume_;
}

} // namespace Sound

} // namespace PanzerChasm
