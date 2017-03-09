#pragma once

#include <shaders_loading.hpp>

#include "size.hpp"

namespace PanzerChasm
{

struct RenderingContextGL
{
	Size2 viewport_size;
	r_GLSLVersion glsl_version;
};

struct RenderingContextSoft
{
	Size2 viewport_size;
	unsigned int row_pixels;

	uint32_t* window_surface_data;
};

} // namespace PanzerChasm
