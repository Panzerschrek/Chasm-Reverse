#pragma once
#include <array>

#include "size.hpp"
#include "vfs.hpp"

namespace PanzerChasm
{

typedef std::array<unsigned char, 768> Palette;

struct CelTextureHeader
{
	unsigned short unknown0;
	unsigned short size[2u];
	unsigned char unknown1[26u];
	Palette palette;
};

static_assert( sizeof(CelTextureHeader) == 800u, "Invalid size" );

void ConvertToRGBA(
	unsigned int pixel_count,
	const unsigned char* in_data,
	const Palette& palette,
	unsigned char* out_data,
	unsigned char transpareny_color_index= 255u );

void FlipAndConvertToRGBA(
	unsigned int width, unsigned int height,
	const unsigned char* in_data,
	const Palette& palette,
	unsigned char* out_data );

void ColorShift(
	unsigned char start_color, unsigned char end_color,
	char shift,
	unsigned int pixel_count,
	const unsigned char* in_data,
	unsigned char* out_data );

void LoadPalette(
	const Vfs& vfs,
	Palette& out_palette );

void CreateConsoleBackground(
	const Size2& size,
	const Vfs& vfs,
	const Palette& palette,
	std::vector<unsigned char>& out_data_rgba );

} // namespace PanzerChasm
