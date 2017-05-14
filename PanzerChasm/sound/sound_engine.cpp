#include <matrix.hpp>

#include "../assert.hpp"
#include "../client/map_state.hpp"
#include "../log.hpp"
#include "../math_utils.hpp"
#include "../settings.hpp"
#include "../shared_settings_keys.hpp"

#include "sound_engine.hpp"

namespace PanzerChasm
{

namespace Sound
{

// Sound constants
static const float g_volume_distance_scale= 4.0f;
static const float g_max_ear_volume= 1.0f;
static const float g_min_ear_volume= 0.1f;
static const float g_ears_angle= 160.0f * Constants::to_rad;
static const float g_ears_half_angle= 0.5f * g_ears_angle;
static const float g_volume_oversaturation_level= 1.4f;
static const float g_ambient_sounds_volume_scale= 0.6f;

SoundEngine::SoundEngine(
	Settings& settings,
	const GameResourcesConstPtr& game_resources )
	: game_resources_( game_resources )
	, settings_( settings )
	, objects_sounds_processor_( game_resources )
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

		sounds_[s]= LoadSound( sound.file_name, *game_resources_->vfs );

		if( sounds_[s] != nullptr )
		{
			total_sounds_loaded++;
			sound_data_size+= sounds_[s]->GetDataSize();
		}
	}

	for( unsigned int i= 0u; i < game_resources_->monsters_models.size() && i < c_max_monsters; i++ )
	{
		const unsigned int first_monster_sound= c_first_monster_sound + i * c_max_monster_sounds;
		const Model& model= game_resources_->monsters_models[i];

		for( unsigned int j= 0; j < model.sounds.size() && j < c_max_monster_sounds; j++ )
		{
			ISoundDataConstPtr& sound= sounds_[ first_monster_sound + j ];
			sound= LoadRawMonsterSound( model.sounds[j] );

			if( sound != nullptr )
			{
				total_sounds_loaded++;
				sound_data_size+= sound->GetDataSize();
			}
		}
	}

	Log::Info( "End loading sounds" );
	Log::Info( "Total ", total_sounds_loaded, " sounds. Sound data size: ", sound_data_size / 1024u, "kb" );
}

SoundEngine::~SoundEngine()
{
	ForceStopAllChannels();
}

void SoundEngine::Tick()
{
	UpdateAmbientSoundState();
	UpdateObjectSoundState();
	UpdateOneTimeSoundSource();
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

			if( !channel.is_active )
				channel.position_samples= 0u;

			channel.is_active= true;
			channel.looped= source.looped;

			if( &source == one_time_sound_source_ )
				channel.src_sound_data= one_time_sound_source_data_.get();
			else
				channel.src_sound_data= sounds_[ source.sound_id ].get();

			// Source is over
			if( channel.is_active &&
				channel.src_sound_data != nullptr &&
				( !channel.looped && channel.position_samples >= channel.src_sound_data->sample_count_ ) )
			{
				source.is_free= true;
				channel.is_active= false;
			}
		}
	}

	driver_.UnlockChannels();
}

void SoundEngine::UpdateMapState( const MapState& map_state )
{
	// Update sounds, linked with mosnters
	const MapState::MonstersContainer& monsters= map_state.GetMonsters();
	for( Source& source : sources_ )
	{
		if( source.is_free )
			continue;

		if( source.monster_id != 0u )
		{
			const auto it= monsters.find( source.monster_id );
			if( it != monsters.end() )
			{
				const MapState::Monster& monster= it->second;
				source.pos= monster.pos;  // TODO - correct z coordinate
			}
		}
	}

	objects_sounds_processor_.Update( map_state, head_position_ );
}

void SoundEngine::SetMap( const MapDataConstPtr& map_data )
{
	ForceStopAllChannels();

	// Reload sounds only if map changed.
	if( map_data != current_map_data_ )
	{
		current_map_data_= map_data;

		if( current_map_data_ != nullptr )
		{
			Log::Info( "Loading sounds for map" );

			// TODO reuse old sounds
			for( unsigned int s= 0u; s < MapData::c_max_map_sounds; s++ )
			{
				ISoundDataConstPtr& sound= sounds_[ c_first_map_sound + s ];
				sound= nullptr; // remove old

				const GameResources::SoundDescription& sound_description= map_data->map_sounds[s];
				if( sound_description.file_name[0] == '\0' )
					continue;

				sound= LoadSound( sound_description.file_name, *game_resources_->vfs );
			}

			for( unsigned int s= 0u; s < MapData::c_max_map_ambients; s++ )
			{
				ISoundDataConstPtr& sound= sounds_[ c_first_map_ambient_sound + s ];
				sound= nullptr; // remove old

				const GameResources::SoundDescription& sound_description= map_data->ambients[s];
				if( sound_description.file_name[0] == '\0' )
					continue;

				sound= LoadSound( sound_description.file_name, *game_resources_->vfs );
			}
		}
	}

	ambient_sound_processor_.SetMap( map_data );
	objects_sounds_processor_.SetMap( map_data );
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

	ambient_sound_processor_.UpdatePosition( position.xy() );
}

void SoundEngine::PlayWorldSound(
	const unsigned int sound_number,
	const m_Vec3& position )
{
	if( sound_number >=
		GameResources::c_max_global_sounds + MapData::c_max_map_sounds + MapData::c_max_map_ambients )
		return;

	Source* const source= GetFreeSource();
	if( source == nullptr )
		return;

	if( sounds_[ sound_number ] == nullptr )
		return;

	source->is_free= false;
	source->looped= false;
	source->sound_id= sound_number;
	source->pos_samples= 0u;
	source->is_head_relative= false;
	source->pos= position;
	source->monster_id= 0u;
}

void SoundEngine::PlayMonsterLinkedSound(
	const MapState::MonstersContainer::value_type& monster_value,
	const unsigned int sound_number )
{
	if( sound_number >= sounds_.size() )
		return;

	Source* const source= GetFreeSource();
	if( source == nullptr )
		return;

	const MapState::Monster& monster= monster_value.second;

	source->is_free= false;
	source->looped= false;
	source->sound_id= sound_number;
	source->pos_samples= 0u;
	source->is_head_relative= false;
	source->pos= monster.pos; // TODO - correct z coordinate
	source->monster_id= monster_value.first;
}

void SoundEngine::PlayMonsterSound(
	const MapState::MonstersContainer::value_type& monster_value,
	const unsigned int monster_sound_id )
{
	const MapState::Monster& monster= monster_value.second;

	if( monster.monster_id >= c_max_monsters ||
		monster_sound_id >= c_max_monster_sounds )
		return;

	Source* const source= GetFreeSource();
	if( source == nullptr )
		return;

	const unsigned int sound_number=
		c_first_monster_sound +
		monster.monster_id * c_max_monster_sounds + monster_sound_id;

	source->is_free= false;
	source->looped= false;
	source->sound_id= sound_number;
	source->pos_samples= 0u;
	source->is_head_relative= false;
	source->pos= monster.pos; // TODO - correct z coordinate
	source->monster_id= monster_value.first;
}

void SoundEngine::PlayHeadSound( const unsigned int sound_number )
{
	if( sound_number >= sounds_.size() )
		return;

	Source* const source= GetFreeSource();
	if( source == nullptr )
		return;

	if( sounds_[ sound_number ] == nullptr )
		return;

	source->is_free= false;
	source->looped= false;
	source->sound_id= sound_number;
	source->pos_samples= 0u;
	source->is_head_relative= true;
	source->monster_id= 0u;
}

void SoundEngine::PlayOneTimeSound( const char* const sound_data_file )
{
	ISoundDataConstPtr sound_data= LoadSound( sound_data_file,* game_resources_->vfs );
	if( sound_data == nullptr )
		return;

	if( one_time_sound_source_ != nullptr ) // Free and kill old sound.
		one_time_sound_source_->is_free= true;

	one_time_sound_source_= GetFreeSource();
	if( one_time_sound_source_ == nullptr )
		return;

	{ // Stop channel, which can play now old OneTimeSoundSource.
		driver_.LockChannels();
		Channels& channels= driver_.GetChannels();
		for( unsigned int i= 0u; i < Channel::c_max_channels; i++ )
		{
			if( &sources_[i] == one_time_sound_source_ )
			{
				channels[i].is_active= false;
				channels[i].src_sound_data= nullptr;
				break;
			}
		}
		driver_.UnlockChannels();
	}

	one_time_sound_source_data_= std::move(sound_data);

	one_time_sound_source_->is_free= false;
	one_time_sound_source_->is_head_relative= true;
	one_time_sound_source_->looped= false;
	one_time_sound_source_->monster_id= 0u;
	one_time_sound_source_->sound_id= 0u;
	one_time_sound_source_->pos_samples= 0u;
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

void SoundEngine::UpdateAmbientSoundState()
{
	const unsigned int sound_number= c_first_map_ambient_sound + ambient_sound_processor_.GetCurrentSoundNumber();
	if( sound_number == c_first_map_ambient_sound )
	{
		if( ambient_sound_source_ != nullptr )
		{
			ambient_sound_source_->is_free= true;
			ambient_sound_source_= nullptr;
		}
	}
	else
	{
		if( ambient_sound_source_ == nullptr )
		{
			ambient_sound_source_= GetFreeSource();
			if( ambient_sound_source_ != nullptr )
			{
				ambient_sound_source_->is_free= false;

				ambient_sound_source_->looped= true;
				ambient_sound_source_->is_head_relative= true;
				ambient_sound_source_->pos_samples= 0u;
				ambient_sound_source_->sound_id= sound_number;
				ambient_sound_source_->monster_id= 0u;
			}
		}
		else
		{
			if( ambient_sound_source_->sound_id != sound_number )
			{
				ambient_sound_source_->sound_id= sound_number;
				ambient_sound_source_->pos_samples= 0u;
			}
		}
	}
}

void SoundEngine::UpdateObjectSoundState()
{
	const unsigned int sound_number= c_first_map_ambient_sound + objects_sounds_processor_.GetCurrentSoundNumber();
	if( sound_number == c_first_map_ambient_sound )
	{
		if( object_sound_source_ != nullptr )
		{
			object_sound_source_->is_free= true;
			object_sound_source_= nullptr;
		}
	}
	else
	{
		if( object_sound_source_ == nullptr )
		{
			object_sound_source_= GetFreeSource();
			if( object_sound_source_ != nullptr )
			{
				object_sound_source_->is_free= false;

				object_sound_source_->looped= true;
				object_sound_source_->is_head_relative= false;
				object_sound_source_->pos_samples= 0u;
				object_sound_source_->pos= objects_sounds_processor_.GetCurrentSoundPosition();
				object_sound_source_->sound_id= sound_number;
				object_sound_source_->monster_id= 0u;
			}
		}
		else
		{
			if( object_sound_source_->sound_id != sound_number )
			{
				object_sound_source_->sound_id= sound_number;
				object_sound_source_->pos_samples= 0u;
			}
			object_sound_source_->pos= objects_sounds_processor_.GetCurrentSoundPosition();
		}
	}
}

void SoundEngine::UpdateOneTimeSoundSource()
{
	if( one_time_sound_source_ != nullptr )
	{
		PC_ASSERT( one_time_sound_source_data_ != nullptr );
		if( one_time_sound_source_->is_free )
		{
			// Free expired sound source.
			one_time_sound_source_data_= nullptr;
			one_time_sound_source_= nullptr;
		}
	}
}

void SoundEngine::CalculateSourcesVolume()
{
	for( Source& source : sources_ )
	{
		if( source.is_free )
			continue;

		unsigned char base_sound_volume_value= GameResources::SoundDescription::c_max_volume;
		if( source.sound_id < GameResources::c_max_global_sounds )
			base_sound_volume_value= game_resources_->sounds[ source.sound_id ].volume;
		else if( current_map_data_ != nullptr )
		{
			if( source.sound_id - c_first_map_sound < MapData::c_max_map_sounds )
				base_sound_volume_value= current_map_data_->map_sounds[ source.sound_id - c_first_map_sound ].volume;
			else if( source.sound_id - c_first_map_ambient_sound < MapData::c_max_map_ambients )
				base_sound_volume_value= current_map_data_->ambients[ source.sound_id - c_first_map_ambient_sound ].volume;
		}

		const float base_sound_volume=
			float( base_sound_volume_value ) / float( GameResources::SoundDescription::c_max_volume );

		if( source.is_head_relative )
			source.volume[0]= source.volume[1]= base_sound_volume;
		else
		{
			const m_Vec3 vec_to_source= source.pos - head_position_;
			const float disntance_to_source= vec_to_source.Length();

			// Linear attenuation.
			const float sound_volume= std::min( base_sound_volume * g_volume_distance_scale / disntance_to_source, g_volume_oversaturation_level );

			for( unsigned int j= 0u; j < 2u; j++ )
			{
				const float angle_cos= ( vec_to_source * ears_vectors_[j] ) / disntance_to_source;
				const float ear_volume=
					0.5f * (
						angle_cos * ( g_max_ear_volume - g_min_ear_volume )
						+ g_min_ear_volume + g_max_ear_volume );

				const float channel_volume= sound_volume * ear_volume;
				source.volume[j]= std::min( std::max( channel_volume, 0.0f ), 1.0f );
			}
		}
	}

	if( ambient_sound_source_ != nullptr )
	{
		const float volume_scale= g_ambient_sounds_volume_scale * ambient_sound_processor_.GetCurrentSoundVolume();
		ambient_sound_source_->volume[0]*= volume_scale;
		ambient_sound_source_->volume[1]*= volume_scale;
	}

	if( object_sound_source_ != nullptr )
	{
		const float volume_scale= g_ambient_sounds_volume_scale * objects_sounds_processor_.GetCurrentSoundVolume();
		object_sound_source_->volume[0]*= volume_scale;
		object_sound_source_->volume[1]*= volume_scale;
	}

	if( one_time_sound_source_ != nullptr )
		one_time_sound_source_->volume[0]= one_time_sound_source_->volume[1]= 1.0f;

	// Read master volume.
	float master_volume= settings_.GetFloat( SettingsKeys::fx_volume, 0.5f );
	master_volume= std::max( 0.0f, std::min( master_volume, 1.0f ) );
	settings_.SetSetting( SettingsKeys::fx_volume, master_volume );

	// Apply master volume.
	for( Source& source : sources_ )
	{
		if( source.is_free )
			continue;
		source.volume[0]*= master_volume;
		source.volume[1]*= master_volume;
	}
}

void SoundEngine::ForceStopAllChannels()
{
	ambient_sound_source_= nullptr;
	object_sound_source_= nullptr;

	one_time_sound_source_data_= nullptr;
	one_time_sound_source_= nullptr;

	// Force stop all channels.
	// This need, because driver life is longer, than life of sound data (global or map).

	driver_.LockChannels();

	Channels& channels= driver_.GetChannels();
	for( Channel& channel : channels )
		channel.is_active= false;

	driver_.UnlockChannels();

	for( Source& source : sources_ )
		source.is_free= true;
}

} // namespace Sound

} // namespace PanzerChasm
