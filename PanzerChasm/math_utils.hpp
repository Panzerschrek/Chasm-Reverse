#pragma once
#include <cmath>
#include <limits>

namespace PanzerChasm
{

namespace Constants
{

const float pi= 3.1415926535f;
const float inv_pi= 1.0f / pi;

const float half_pi= 0.5f * pi;
const float two_pi= 2.0f * pi;

const float to_rad= pi / 180.0f;
const float to_deg= 180.0f / pi;

const float max_float= std::numeric_limits<float>::max();
const float min_float= -max_float;

} // namespace Contants

} // namespace PanzerChasm
