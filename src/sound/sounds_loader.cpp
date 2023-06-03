#include <cstring>

#include <SDL_audio.h>

#include "../assert.hpp"
#include "../log.hpp"
#include "../vfs.hpp"

#include "sounds_loader.hpp"
#include "common/files.hpp"

namespace PanzerChasm
{

namespace Sound
{

namespace
{

class RawPCMSoundData final : public ISoundData
{
public:
	RawPCMSoundData( Vfs::FileContent data )
	{
		pcm_data_= std::move( data );

		frequency_= 11025u;
		data_type_= DataType::Unsigned8;
		data_= pcm_data_.data();
		sample_count_= pcm_data_.size();
	}

	virtual ~RawPCMSoundData() override {}

private:
	Vfs::FileContent pcm_data_;
};

class RawMonsterSoundData final : public ISoundData
{
public:
	RawMonsterSoundData( Vfs::FileContent data )
	{
		pcm_data_= std::move( data );

		frequency_= 11025u;
		data_type_= DataType::Signed8;
		data_= pcm_data_.data();
		sample_count_= pcm_data_.size();
	}

	virtual ~RawMonsterSoundData() override {}

private:
	Vfs::FileContent pcm_data_;
};


class WavSoundData final : public ISoundData
{
public:
	WavSoundData( const Vfs::FileContent& data )
	{
		bool ok= false;

		SDL_AudioSpec spec;
		spec.freq= 0;
		spec.format= 0;
		spec.channels= 0;
		Uint32 buffer_length_butes;

		SDL_RWops* const rw_ops=
			SDL_RWFromConstMem( data.data(), data.size() );

		if( rw_ops != nullptr )
		{
			const SDL_AudioSpec* const result=
				SDL_LoadWAV_RW(
					rw_ops, 1, // 1 - means free rw_ops
					&spec,
					&wav_buffer_, &buffer_length_butes );

			ok= result != nullptr;
			PC_ASSERT( result == nullptr || result == &spec );
		}

		const unsigned int bits= SDL_AUDIO_BITSIZE( spec.format );
		const bool is_signed= SDL_AUDIO_ISSIGNED( spec.format ) != 0;

		if( !( bits == 16u || bits == 8u ) )
			ok= false;
		if( SDL_AUDIO_ISFLOAT( spec.format ) || spec.channels != 1u )
			ok= false;

		if( ok )
		{
			frequency_= spec.freq;
			data_type_=
				bits == 8u
					? ( is_signed ? DataType::Signed8  : DataType::Unsigned8  )
					: ( is_signed ? DataType::Signed16 : DataType::Unsigned16 );
			data_= wav_buffer_;
			sample_count_= buffer_length_butes / ( bits / 8u );
		}
		else
		{
			// Make dummy
			wav_buffer_= nullptr;
			frequency_= 22050;
			data_type_= DataType::Unsigned8;
			data_= nullptr;
			sample_count_= 0u;
		}
	}

	virtual ~WavSoundData() override
	{
		if( wav_buffer_ != nullptr )
			SDL_FreeWAV( wav_buffer_ );
	}

private:
	Uint8* wav_buffer_= nullptr;
};

} // namespace

ISoundDataConstPtr LoadSound( const char* file_path, Vfs& vfs )
{
	Vfs::FileContent file_content= vfs.ReadFile( file_path );
	if( file_content.empty() )
	{
		Log::Warning( "Can not load \"", file_path, "\"" );
		return nullptr;
	}

	const char* const extension= ChasmReverse::ExtractExtension( file_path );

	if( std::strcmp( extension, "WAV" ) == 0 ||
		std::strcmp( extension, "wav" ) == 0 )
	{
		return ISoundDataConstPtr( new WavSoundData( file_content ) );
	}
	else // *.SFX, *.PCM, *.RAW files
		return ISoundDataConstPtr( new RawPCMSoundData( std::move( file_content ) ) );

	return nullptr;
}

ISoundDataConstPtr LoadRawMonsterSound( const Vfs::FileContent& raw_sound_data )
{
	if( raw_sound_data.empty() )
		return nullptr;

	return ISoundDataConstPtr( new RawMonsterSoundData( raw_sound_data ) );
}

} // namespace Sound

} // namespace PanzerChasm
