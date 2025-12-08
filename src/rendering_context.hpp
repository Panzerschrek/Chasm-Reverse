#pragma once
#include <cstdint>
#include <memory>

#include <shaders_loading.hpp>

#include "size.hpp"

namespace PanzerChasm
{

struct RenderingContextGL
{
	Size2 viewport_size;
	r_GLSLVersion glsl_version;
};

typedef std::array< uint32_t, 256u > PaletteTransformed;
typedef std::shared_ptr< PaletteTransformed > PaletteTransformedPtr;

struct RenderingContextSoft
{
	Size2 viewport_size;
	unsigned int row_pixels;

	uint32_t* window_surface_data;

	unsigned char color_indeces_rgba[4];

	PaletteTransformedPtr palette_transformed;
};

} // namespace PanzerChasm
