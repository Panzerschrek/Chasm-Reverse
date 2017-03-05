#include "constants.glsl"

uniform sampler2D tex;
uniform sampler2D lightmap;

in vec2 f_tex_coord;
in vec2 f_lightmap_coord;
in float f_alpha_test_mask;
in float f_discard_mask;

out vec4 color;

void main()
{
	vec4 c= texture( tex, f_tex_coord );

	// TODO - optimize discard operation here
	if( f_discard_mask < 0.5f || ( c.a < 0.5 && f_alpha_test_mask > 0.5 ) )
		discard;

	float light= c_static_light_scale * texture( lightmap, f_lightmap_coord ).x;
	color= vec4( light * c.xyz, 0.25 );
}
