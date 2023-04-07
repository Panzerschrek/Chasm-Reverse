#include <algorithm>

#include "game_constants.hpp"

#include "menu_drawers_common.hpp"

namespace PanzerChasm
{

unsigned int CalculateMenuScale( const Size2& viewport_size )
{
	float scale_f=
		std::min(
			float( viewport_size.Width () ) / float( GameConstants::min_screen_width  ),
			float( viewport_size.Height() ) / float( GameConstants::min_screen_height ) );

	 // Do not scale menu too height.
	if( scale_f > 3.0f )
		scale_f*= 1.0f - 0.25f * std::min( scale_f - 3.0f, 1.0f );

	return std::max( 1u, static_cast<unsigned int>( scale_f ) );
}

unsigned int CalculateConsoleScale( const Size2& viewport_size )
{
	float scale_f=
		std::min(
			float( viewport_size.Width () ) / float( GameConstants::min_screen_width  ),
			float( viewport_size.Height() ) / float( GameConstants::min_screen_height ) );

	scale_f*= 0.75f; // Do not scale console too height

	const unsigned int scale_i= std::max( 1u, static_cast<unsigned int>( scale_f ) );

	// Find nearest powert of two scale, lowest, then scale_i.
	unsigned int scale_log2= 0u;
	while( scale_i >= ( 1u << scale_log2 ) )
		scale_log2++;
	scale_log2--;

	return 1u << scale_log2;
}

} // namespace PanzerChasm
