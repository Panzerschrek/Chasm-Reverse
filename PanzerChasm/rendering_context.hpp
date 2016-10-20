#pragma once

#include <shaders_loading.hpp>

#include "size.hpp"

namespace PanzerChasm
{

struct RenderingContext
{
	Size2 viewport_size;
	r_GLSLVersion glsl_version;
};

} // namespace PanzerChasm
