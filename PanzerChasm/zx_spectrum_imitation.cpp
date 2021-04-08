#include <algorithm>
#include "zx_spectrum_imitation.hpp"


namespace PanzerChasm
{

namespace
{

struct ColorUnbounded
{
	int32_t components[3];
};

ColorUnbounded UnpackColor(const uint32_t color)
{
	ColorUnbounded res;
	res.components[0]= (color >>  0) & 0xFF;
	res.components[1]= (color >>  8) & 0xFF;
	res.components[2]= (color >> 16) & 0xFF;
	return res;
}

uint32_t PackColor(ColorUnbounded color)
{
	return color.components[0] | (color.components[1] << 8) | (color.components[2] << 16);
}

int32_t ColorSquareDistance(const ColorUnbounded& c0, const ColorUnbounded& c1)
{
	const int32_t d[3]={ c0.components[0] - c1.components[0], c0.components[1] - c1.components[1], c0.components[2] - c1.components[2] };
	return d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
}

ColorUnbounded GetColorForIndex(const uint32_t index, const uint32_t brightness_bit)
{
	if(brightness_bit == 0)
	{
		static constexpr ColorUnbounded table[8]=
		{
			{ { 0x00, 0x00, 0x00 } },
			{ { 0x00, 0x00, 0xD7 } },
			{ { 0xD7, 0x00, 0x00 } },
			{ { 0xD7, 0x00, 0xD7 } },
			{ { 0x00, 0xD7, 0x00 } },
			{ { 0x00, 0xD7, 0xD7 } },
			{ { 0xD7, 0xD7, 0x00 } },
			{ { 0xD7, 0xD7, 0xD7 } },
		};
		return table[index];
	}
	else
	{
		static constexpr ColorUnbounded table[8]=
		{
			{ { 0x00, 0x00, 0x00 } },
			{ { 0x00, 0x00, 0xFF } },
			{ { 0xFF, 0x00, 0x00 } },
			{ { 0xFF, 0x00, 0xFF } },
			{ { 0x00, 0xFF, 0x00 } },
			{ { 0x00, 0xFF, 0xFF } },
			{ { 0xFF, 0xFF, 0x00 } },
			{ { 0xFF, 0xFF, 0xFF } },
		};
		return table[index];
	}
}

} // namespace

void TransformImageToZXSpectrumLike(
	uint32_t* const pixels,
	const uint32_t width,
	const uint32_t height,
	const uint32_t row_size)
{
	const uint32_t tile_size= 8;
	for(uint32_t tile_y= 0; tile_y < height; tile_y +=tile_size)
	for(uint32_t tile_x= 0; tile_x < width ; tile_x +=tile_size)
	{
		// First step, try all combinations of colors to find optimal.
		uint32_t best_color_index= 0;
		int32_t best_color_deviation = std::numeric_limits<int32_t>::max();
		for(uint32_t bridgtness_bit= 0; bridgtness_bit < 2; ++bridgtness_bit)
		for(uint32_t color_index_0= 0; color_index_0 < 8; ++color_index_0)
		for(uint32_t color_index_1= color_index_0 + 1; color_index_1 < 8; ++color_index_1)
		{
			const ColorUnbounded color0= GetColorForIndex(color_index_0, bridgtness_bit);
			const ColorUnbounded color1= GetColorForIndex(color_index_1, bridgtness_bit);

			// Try this combination for all tile pixels.
			int32_t color_deviation= 0;
			for(uint32_t y= tile_y; y < tile_y + tile_size; ++y)
			for(uint32_t x= tile_x; x < tile_x + tile_size; ++x)
			{
				const ColorUnbounded color= UnpackColor(pixels[x + y * row_size]);

				// Try all variations of dithered color.
				int32_t best_pixel_square_distance= std::numeric_limits<int32_t>::max();
				for(uint32_t i= 0; i <= 4; ++i)
				{
					ColorUnbounded mixed_color;
					for(uint32_t comp= 0; comp < 3; ++comp)
						mixed_color.components[comp]=
							(color0.components[comp] * (4 - i) + color1.components[comp] * i) / 4;

					best_pixel_square_distance= std::min(best_pixel_square_distance, ColorSquareDistance(color, mixed_color));
				}
				color_deviation+= best_pixel_square_distance;
			}

			if(color_deviation < best_color_deviation)
			{
				best_color_index= color_index_0 | (color_index_1 << 3) | (bridgtness_bit << 6);
				best_color_deviation= color_deviation;
			}
		}

		// Now fetch colors for best combination.

		const uint32_t color_index_0= (best_color_index >> 0) & 7;
		const uint32_t color_index_1= (best_color_index >> 3) & 7;
		const uint32_t brightness_bit= best_color_index >> 6;

		const ColorUnbounded color0= GetColorForIndex(color_index_0, brightness_bit);
		const ColorUnbounded color1= GetColorForIndex(color_index_1, brightness_bit);

		// Finally apply selected color to pixels of this tile.
		for(uint32_t y= tile_y; y < tile_y + tile_size; ++y)
		for(uint32_t x= tile_x; x < tile_x + tile_size; ++x)
		{
			uint32_t& dst= pixels[x + y * row_size];
			const ColorUnbounded src_color= UnpackColor(dst);
			const ColorUnbounded result_color= ColorSquareDistance(src_color, color0) < ColorSquareDistance(src_color, color1) ? color0 : color1;
			dst= PackColor(result_color);
		}
	}
}

} // namespace PanzerChasm
