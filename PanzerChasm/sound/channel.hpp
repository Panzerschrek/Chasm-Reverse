#pragma once
#include <memory>
#include <vector>

namespace PanzerChasm
{

namespace Sound
{

typedef short SampleType;

struct Channel
{
	static constexpr unsigned int c_max_channels= 32u;
	static constexpr unsigned int  c_left_channel_number= 0u;
	static constexpr unsigned int c_right_channel_number= 1u;

	float volume[2]; // right/left volume

	unsigned int position_samples;

	struct
	{
		SampleType* data;
		unsigned int size_samples;
	} src_sound_data;
};

typedef std::vector<Channel> Channels;

} // namespace Sound

} // namespace PanzerChasm
