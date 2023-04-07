#pragma once

namespace PanzerChasm
{

class ITextDrawer
{
public:
	enum class FontColor
	{
		White= 0u,
		DarkYellow,
		Golden,
		YellowGreen,
		Green,
	};

	enum class Alignment
	{
		Left,
		Center,
		Right,
	};

	static constexpr unsigned char c_slider_back_letter_code= 8u;
	static constexpr unsigned char c_slider_left_letter_code = 1u;
	static constexpr unsigned char c_slider_right_letter_code= 2u;
	static constexpr unsigned char c_slider_letter_code= 3u;

public:
	virtual ~ITextDrawer(){}

	virtual unsigned int GetLineHeight() const= 0;

	virtual void Print(
		int x, int y,
		const char* text,
		unsigned int scale,
		FontColor color= FontColor::White,
		Alignment alignment= Alignment::Left )= 0;
};

} // namespace PanzerChasm
