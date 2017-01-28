#include "constants.glsl"

uniform sampler2DArray tex;
uniform sampler2D lightmap;

in vec3 f_tex_coord;
in vec2 f_lightmap_coord;
in float f_alpha_test_mask;

out vec4 color;

void main()
{
	vec4 c= texture( tex, f_tex_coord );
	if( c.a < 0.5 && f_alpha_test_mask > 0.5 )
		discard;

	float light= c_light_scale * texture( lightmap, f_lightmap_coord ).x;
	color= vec4( light * c.xyz, 0.25 );
}
