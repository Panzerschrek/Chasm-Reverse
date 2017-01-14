#pragma once
#include <array>
#include <memory>

namespace PanzerChasm
{

namespace Sound
{

typedef short SampleType;
class ISoundData;

struct Channel
{
	static constexpr unsigned int c_max_channels= 32u;
	static constexpr unsigned int  c_left_channel_number= 0u;
	static constexpr unsigned int c_right_channel_number= 1u;

	bool is_active= false;
	float volume[2]; // right/left volume

	unsigned int position_samples;
	bool looped;

	ISoundData* src_sound_data= nullptr;
};

typedef std::array<Channel, Channel::c_max_channels> Channels;

} // namespace Sound

} // namespace PanzerChasm
