#include <limits>

#include <SDL.h>

#include "../assert.hpp"
#include "../log.hpp"

#include "sounds_loader.hpp"

#include "driver.hpp"

namespace PanzerChasm
{

namespace Sound
{

static const SDL_AudioDeviceID g_first_valid_device_id= 2u;
static const unsigned int g_left_and_right= 2u;

static int NearestPowerOfTwoFloor( int x )
{
	int r= 1 << 30;
	while( r > x ) r>>= 1;
	return r;
}

static const unsigned int g_frac_bits= 10u;
static const unsigned int g_frac= 1u << g_frac_bits;
static const unsigned int g_frac_minus_one= g_frac - 1u;
static_assert( g_frac_bits >= 8u, "Too low fractional bits" );

const int g_volume_bits= 8;
const int g_volume_scale= 1 << g_volume_bits;

Driver::Driver()
{
	SDL_InitSubSystem( SDL_INIT_AUDIO );

	SDL_AudioSpec requested_format;
	SDL_AudioSpec obtained_format;

	requested_format.channels= g_left_and_right;
	requested_format.freq= 22050;
	requested_format.format= AUDIO_S16;
	requested_format.callback= AudioCallback;
	requested_format.userdata= this;

	// ~ 1 callback call per two frames (60fps)
	requested_format.samples= NearestPowerOfTwoFloor( requested_format.freq / 30 );

	int device_count= SDL_GetNumAudioDevices(0);
	// Can't get explicit devices list. Trying to use first device.
	if( device_count == -1 )
		device_count= 1;

	for( int i= 0; i < device_count; i++ )
	{
		const char* const device_name= SDL_GetAudioDeviceName( i, 0 );

		const SDL_AudioDeviceID device_id=
			SDL_OpenAudioDevice( device_name, 0, &requested_format, &obtained_format, 0 );

		if( device_id >= g_first_valid_device_id &&
			obtained_format.channels == requested_format.channels &&
			obtained_format.format   == requested_format.format )
		{
			device_id_= device_id;
			Log::Info( "Open audio device: ", device_name );
			break;
		}
	}

	if( device_id_ < g_first_valid_device_id )
	{
		Log::FatalError( "Can not open any audio device" );
		return;
	}

	frequency_= obtained_format.freq;

	mix_buffer_.resize( obtained_format.samples * g_left_and_right );

	// Run
	SDL_PauseAudioDevice( device_id_ , 0 );
}

Driver::~Driver()
{
	if( device_id_ >= g_first_valid_device_id )
		SDL_CloseAudioDevice( device_id_ );


	SDL_QuitSubSystem( SDL_INIT_AUDIO );
}

void Driver::LockChannels()
{
	if( device_id_ >= g_first_valid_device_id )
		SDL_LockAudioDevice( device_id_ );
}

void Driver::UnlockChannels()
{
	if( device_id_ >= g_first_valid_device_id )
		SDL_UnlockAudioDevice( device_id_ );
}

Channels& Driver::GetChannels()
{
	return channels_;
}

void SDLCALL Driver::AudioCallback( void* userdata, Uint8* stream, int len_bytes )
{
	Driver* const self= reinterpret_cast<Driver*>( userdata );

	self->FillAudioBuffer(
		reinterpret_cast<SampleType*>( stream ),
		static_cast<unsigned int>( len_bytes ) / ( sizeof(SampleType) * g_left_and_right ) );
}

void Driver::FillAudioBuffer( SampleType* const buffer, const unsigned int sample_count )
{
	PC_ASSERT( sample_count * g_left_and_right <= mix_buffer_.size() );

	// Zero mix buffer.
	for( unsigned int i= 0u; i < sample_count * g_left_and_right; i++ )
		mix_buffer_[i]= 0;

	for( Channel& channel : channels_ )
	{
		if( !channel.is_active || channel.src_sound_data == nullptr )
			continue;

		// Fill destination audio buffer with iterpolation,
		// because original game sounds have frequency about 11025 Hz, but destination buffer have frequency 22050 - 44100 Hz.
		const unsigned int freq_ratio_f= ( channel.src_sound_data->frequency_ << g_frac_bits ) / frequency_;
		const int volume[2]=
		{
			static_cast<int>( channel.volume[0] * float( g_volume_scale ) ),
			static_cast<int>( channel.volume[1] * float( g_volume_scale ) ),
		};

		unsigned int channel_dst_samples_processed= 0u;
		do
		{
			const auto max_dst_sample_for_src_sample=
			[&]( const unsigned int i ) -> unsigned int
			{
				if( i == 0u ) return 0u;
				const unsigned int result= ( ( i << g_frac_bits ) - 1u ) / freq_ratio_f;
				PC_ASSERT( ( ( result * freq_ratio_f ) >> g_frac_bits ) < i );
				return result + 1u;
			};

			int* const dst= mix_buffer_.data() + channel_dst_samples_processed * g_left_and_right;
			const unsigned int can_read_src_samples= std::max( int( channel.src_sound_data->sample_count_ - channel.position_samples ) - 1, 0 );
			const unsigned int dst_samples_to_write=
				std::min(
					sample_count - channel_dst_samples_processed,
					max_dst_sample_for_src_sample( can_read_src_samples ) );

			switch( channel.src_sound_data->data_type_ )
			{
			case ISoundData::DataType::Unsigned8:
				{
					const unsigned char* const src=
						static_cast<const unsigned char*>( channel.src_sound_data->data_ ) + channel.position_samples;

					for( unsigned int i= 0u; i < dst_samples_to_write; i++ )
					{
						const unsigned int sample_coord_f= i * freq_ratio_f;
						const unsigned int sample_coord= sample_coord_f >> g_frac_bits;
						const unsigned int part= sample_coord_f & ( g_frac - 1u );
						PC_ASSERT( sample_coord + 1u < channel.src_sound_data->sample_count_ - channel.position_samples );

						// Value in range [ 0; 255 * g_frac ]
						const unsigned int mixed_src_sample=
							src[ sample_coord ] * ( g_frac - part ) + src[ sample_coord + 1u ] * part;
						const int signed_sample= int( mixed_src_sample ) - ( 128 << g_frac_bits );

						dst[ i * 2u      ]+= ( signed_sample * volume[0] ) >> ( int(g_frac_bits) + g_volume_bits - 8 );
						dst[ i * 2u + 1u ]+= ( signed_sample * volume[1] ) >> ( int(g_frac_bits) + g_volume_bits - 8  );
					}
				}
				break;

			case ISoundData::DataType::Signed8:
			{
				const char* const src=
					static_cast<const char*>( channel.src_sound_data->data_ ) + channel.position_samples;

				for( unsigned int i= 0u; i < dst_samples_to_write; i++ )
				{
					const unsigned int sample_coord_f= i * freq_ratio_f;
					const unsigned int sample_coord= sample_coord_f >> g_frac_bits;
					const unsigned int part= sample_coord_f & ( g_frac - 1u );
					PC_ASSERT( sample_coord + 1u < channel.src_sound_data->sample_count_ - channel.position_samples );

					// Value in range [ -128 * g_frac; 127 * g_frac ]
					const int signed_sample=
						int( src[ sample_coord ] * ( g_frac - part ) ) + int( src[ sample_coord + 1u ] * part );

					dst[ i * 2u      ]+= ( signed_sample * volume[0] ) >> ( int(g_frac_bits) + g_volume_bits - 8 );
					dst[ i * 2u + 1u ]+= ( signed_sample * volume[1] ) >> ( int(g_frac_bits) + g_volume_bits - 8 );
				}
			}
			break;

			case ISoundData::DataType::Signed16:
			case ISoundData::DataType::Unsigned16:
				PC_ASSERT( false ); // TODO
				break;
			};

			channel.position_samples+= ( ( sample_count - channel_dst_samples_processed ) * freq_ratio_f ) >> g_frac_bits;
			channel.position_samples= std::min( channel.position_samples, channel.src_sound_data->sample_count_ );
			channel_dst_samples_processed+= dst_samples_to_write;

			if( channel.looped &&
				( channel.position_samples >= channel.src_sound_data->sample_count_ || dst_samples_to_write == 0u ) )
				channel.position_samples= 0u;

			} while( channel.looped && channel_dst_samples_processed < sample_count );
	} // for channels

	// Copy mix buffer to result buffer.
	for( unsigned int i= 0u; i < sample_count * g_left_and_right; i++ )
	{
		int s= mix_buffer_[i];
		if( s > +32767 ) s= +32767;
		if( s < -32767 ) s= -32767;
		buffer[i]= s;
	}
}

} // namespace Sound

} // namespace PanzerChasm
