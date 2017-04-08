#pragma once
#include "../size.hpp"
#include "game_constants.hpp"

namespace PanzerChasm
{

namespace MinimapParams
{

// Colors
const unsigned char walls_color= 15u * 16u + 8u;
const unsigned char arrow_color= 10u * 16u + 3u;
const unsigned char framing_color= 0u * 16u + 8u;
const unsigned char framing_contour_color= 0u * 16u + 0u;

// Minimap viewport params
const float left_offset_px= 16.0f;
const float top_offset_px= 16.0f;
const float bottom_offset_rel= 1.0f / 3.0f;
const float right_offset_from_center_px= 32.0f;

} // MinimapParams

inline unsigned int CalculateMinimapScale( const Size2& viewport_size )
{
	float scale_f=
		0.65f * std::min(
			float( viewport_size.Width () ) / float( GameConstants::min_screen_width  ),
			float( viewport_size.Height() ) / float( GameConstants::min_screen_height ) );

	return std::max( 1u, static_cast<unsigned int>( scale_f ) );
}

} // namespace PanzerChasm
