#include "constants.glsl"

uniform sampler2DArray tex;
uniform sampler2D lightmap;

in vec3 f_tex_coord;
in vec2 f_lightmap_coord;
in vec3 f_normal;
in float f_alpha_test_mask;

out vec4 color;

void main()
{
	vec4 c= texture( tex, f_tex_coord );
	if( c.a < 0.5 && f_alpha_test_mask > 0.5 )
		discard;

	float normal_xy_square_length= dot( f_normal.xy, f_normal.xy );

	// Shift fetch position towards normal. This is hack for some map models.
	vec2 lightmap_fetch_coord= f_lightmap_coord + f_normal.xy / ( 64.0 * 7.9 );
	vec4 lightmap_value= texture( lightmap, lightmap_fetch_coord );
	float light_for_flat_surface= 0.5 * length( lightmap_value );

	float directional_light=
		max( 0.0, +lightmap_value.x * f_normal.x * normal_xy_square_length ) +
		max( 0.0, -lightmap_value.y * f_normal.x * normal_xy_square_length ) +
		max( 0.0, +lightmap_value.z * f_normal.y * normal_xy_square_length ) +
		max( 0.0, -lightmap_value.w * f_normal.y * normal_xy_square_length ) +
		f_normal.z * f_normal.z * light_for_flat_surface;

	float light= c_light_scale * directional_light;
	color= vec4( light * c.xyz, 0.25 );
}
