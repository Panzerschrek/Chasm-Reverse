#include <matrix.hpp>

#include "../assert.hpp"
#include "../log.hpp"
#include "../math_utils.hpp"

#include "sound_engine.hpp"

namespace PanzerChasm
{

namespace Sound
{

// Sound constants
static const float g_volume_distance_scale= 8.0f;
static const float g_max_ear_volume= 1.0f;
static const float g_min_ear_volume= 0.1f;
static const float g_ears_angle= 160.0f * Constants::to_rad;
static const float g_ears_half_angle= 0.5f * g_ears_angle;

SoundEngine::SoundEngine( const GameResourcesConstPtr& game_resources )
	: game_resources_( game_resources )
{
	PC_ASSERT( game_resources_ != nullptr );

	Log::Info( "Start loading sounds" );

	unsigned int total_sounds_loaded= 0u;
	unsigned int sound_data_size= 0u;

	for( unsigned int s= 0u; s < GameResources::c_max_global_sounds; s++ )
	{
		const GameResources::SoundDescription& sound= game_resources_->sounds[s];
		if( sound.file_name[0] == '\0' )
			continue;

		global_sounds_[s]= LoadSound( sound.file_name, *game_resources_->vfs );

		if( global_sounds_[s] != nullptr )
		{
			total_sounds_loaded++;
			sound_data_size+= global_sounds_[s]->GetDataSize();
		}
	}

	Log::Info( "End loading sounds" );
	Log::Info( "Total ", total_sounds_loaded, " sounds. Sound data size: ", sound_data_size / 1024u, "kb" );
}

SoundEngine::~SoundEngine()
{
	// Force stop all channels.
	// This need, because driver life is longer, than life of sound data.

	driver_.LockChannels();

	Channels& channels= driver_.GetChannels();
	for( Channel& channel : channels )
		channel.is_active= false;

	driver_.UnlockChannels();
}

void SoundEngine::Tick()
{
	CalculateSourcesVolume();

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
			channel.volume[0]= source.volume[0];
			channel.volume[1]= source.volume[1];

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
	head_position_= position;

	m_Vec3 vectors[2];
	// Left
	vectors[0].x= std::sin( -g_ears_half_angle );
	vectors[0].y= std::cos( -g_ears_half_angle );
	vectors[0].z= 0.0f;
	// Right
	vectors[1].x= std::sin( +g_ears_half_angle );
	vectors[1].y= std::cos( +g_ears_half_angle );
	vectors[1].z= 0.0f;

	m_Mat4 rotate, rotate_z, rotate_x;
	rotate_x.RotateX( x_angle );
	rotate_z.RotateZ( z_angle );
	rotate= rotate_x * rotate_z;

	for( unsigned int j= 0u; j < 2u; j++ )
		ears_vectors_[j]= vectors[j] * rotate;
}

void SoundEngine::PlayWorldSound(
	const unsigned int sound_number,
	const m_Vec3& position )
{
	Source* const source= GetFreeSource();
	if( source == nullptr )
		return;

	if( global_sounds_[ sound_number ] == nullptr )
		return;

	source->is_free= false;
	source->sound_id= sound_number;
	source->pos_samples= 0u;
	source->is_head_relative= false;
	source->pos= position;
}

void SoundEngine::PlayHeadSound( const unsigned int sound_number )
{
	Source* const source= GetFreeSource();
	if( source == nullptr )
		return;

	if( global_sounds_[ sound_number ] == nullptr )
		return;

	source->is_free= false;
	source->sound_id= sound_number;
	source->pos_samples= 0u;
	source->is_head_relative= true;
}

SoundEngine::Source* SoundEngine::GetFreeSource()
{
	for( Source& s : sources_ )
	{
		if( s.is_free )
			return &s;
	}

	return nullptr;
}

void SoundEngine::CalculateSourcesVolume()
{
	for( Source& source : sources_ )
	{
		if( source.is_free )
			continue;

		const float base_sound_volume=
			float( game_resources_->sounds[ source.sound_id ].volume ) /
			float( GameResources::SoundDescription::c_max_volume );

		if( source.is_head_relative )
			source.volume[0]= source.volume[1]= base_sound_volume;
		else
		{
			const m_Vec3 vec_to_source= source.pos - head_position_;
			const float disntance_to_source= vec_to_source.Length();

			for( unsigned int j= 0u; j < 2u; j++ )
			{
				const float angle_cos= ( vec_to_source * ears_vectors_[j] ) / disntance_to_source;
				const float ear_volume=
					0.5f * (
						angle_cos * ( g_max_ear_volume - g_min_ear_volume )
						+ g_min_ear_volume + g_max_ear_volume );

				// Quadratic attenuation.
				const float volume=
					base_sound_volume * g_volume_distance_scale * ear_volume /
					( disntance_to_source * disntance_to_source );

				source.volume[j]= std::min( std::max( volume, 0.0f ), 1.0f );
			}
		}
	}
}

} // namespace Sound

} // namespace PanzerChasm
