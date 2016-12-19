#include "../assert.hpp"

#include "sound_engine.hpp"

namespace PanzerChasm
{

namespace Sound
{

SoundEngine::SoundEngine( const GameResourcesConstPtr& game_resources )
	: game_resources_( game_resources )
{
	PC_ASSERT( game_resources_ != nullptr );

	for( unsigned int s= 0u; s < GameResources::c_max_global_sounds && s < 20u; s++ )
	{
		const GameResources::SoundDescription& sound= game_resources_->sounds[s];
		if( sound.file_name[0] == '\0' )
			continue;

		global_sounds_[s]= LoadSound( sound.file_name, *game_resources_->vfs );
	}
}

SoundEngine::~SoundEngine()
{}

void SoundEngine::Tick()
{
	driver_.LockChannels();

	Channels& channels= driver_.GetChannels();

	for( unsigned int i= 0u; i < Channel::c_max_channels; i++ )
	{
		Channel& channel= channels[i];
		Source& source= sources_[i];

		if( source.is_free )
			channel.is_active= false;
		else
		{
			channel.volume[0]= channel.volume[1]= 1.0f;

			// Channel not active - fill data for channel
			if( !channel.is_active )
			{
				channel.is_active= true;
				channel.position_samples= 0u;
				channel.src_sound_data= global_sounds_[ source.sound_id ].get();
			}

			// Source is over
			if( channel.is_active &&
				channel.src_sound_data != nullptr &&
				channel.position_samples >= channel.src_sound_data->sample_count_ )
			{
				source.is_free= true;
				channel.is_active= false;
			}
		}
	}

	driver_.UnlockChannels();
}

void SoundEngine::SetMap( const MapDataPtr& map_data )
{
	current_map_data_= map_data;
}

void SoundEngine::SetHeadPosition(
	const m_Vec3& position,
	const float z_angle, const float x_angle )
{
	PC_UNUSED( position );
	PC_UNUSED( z_angle );
	PC_UNUSED( x_angle );
}

void SoundEngine::PlayWorldSound(
	const unsigned int sound_number,
	const m_Vec3& position )
{
	PC_UNUSED( sound_number );
	PC_UNUSED( position );
}

void SoundEngine::PlayHeadSound( const unsigned int sound_number )
{
	Source* source= nullptr;
	for( Source& s : sources_ )
	{
		if( s.is_free )
		{
			source= &s;
			break;
		}
	}

	if( source == nullptr )
		return;

	if( global_sounds_[ sound_number ] == nullptr )
		return;

	source->is_free= false;
	source->sound_id= sound_number;
	source->pos_samples= 0u;
}

} // namespace Sound

} // namespace PanzerChasm
