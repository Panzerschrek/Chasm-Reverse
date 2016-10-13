#pragma once
#include <array>

namespace ChasmReverse
{

typedef std::array<unsigned char, 768u> Palette;

void LoadPalette( Palette& out_palette );

} // namespace ChasmReverse
