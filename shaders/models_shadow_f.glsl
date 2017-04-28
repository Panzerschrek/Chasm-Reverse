#include "constants.glsl"

in float f_discard_mask; // Polygon discard

out vec4 color;

void main()
{
	if( f_discard_mask < 0.5f )
		discard;

	color= vec4( 0.0, 0.0, 0.0, c_shadows_alpha );
}
