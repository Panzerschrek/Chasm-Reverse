#include "zx_spectrum_imitation.hpp"


namespace PanzerChasm
{

void TransformImageToZXSpectrumLike(
	uint32_t* const pixels,
	const uint32_t width,
	const uint32_t height,
	const uint32_t row_size)
{
	for(uint32_t y= 0; y < height; ++y)
	for(uint32_t x= 0; x < width; ++x)
		pixels[x + y * row_size]^= 0xFFFFFFFF;
}

} // namespace PanzerChasm
