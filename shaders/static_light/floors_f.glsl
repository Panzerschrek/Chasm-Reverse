#include "constants.glsl"

uniform sampler2DArray tex;
uniform sampler2D lightmap;

in vec3 f_tex_coord;
in vec2 f_lightmap_coord;

out vec4 color;

void main()
{
	float light= c_static_light_scale * texture( lightmap, f_lightmap_coord ).x;
	color= texture( tex, f_tex_coord ) * light;
}
