#include "constants.glsl"

uniform sampler2DArray tex;
uniform sampler2D lightmap;

in vec3 f_tex_coord;
in vec2 f_lightmap_coord;

out vec4 color;

void main()
{
	vec4 light_basis= texture( lightmap, f_lightmap_coord );
	float light_for_flat_surface= 0.5 * length( light_basis );

	float light= c_light_scale * light_for_flat_surface;
	color= texture( tex, f_tex_coord ) * light;
}
