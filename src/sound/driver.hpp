#pragma once
#include <vector>

#include <SDL_audio.h>

#include "channel.hpp"

namespace PanzerChasm
{

namespace Sound
{

class Driver final
{
public:
	Driver();
	~Driver();

	void LockChannels();
	void UnlockChannels();
	Channels& GetChannels();

private:
	static void SDLCALL AudioCallback( void* userdata, Uint8* stream, int len_bytes );
	void FillAudioBuffer( SampleType* buffer, unsigned int sample_count );

private:
	Channels channels_;

	SDL_AudioDeviceID device_id_= 0u;
	unsigned int frequency_; // samples per second

	std::vector<int> mix_buffer_;
};

} // namespace Sound

} // namespace PanzerChasm
