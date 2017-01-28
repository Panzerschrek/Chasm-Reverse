#include "constants.glsl"

uniform sampler2D tex;
uniform sampler2D lightmap;

in vec2 f_tex_coord;
in vec2 f_lightmap_coord;
in vec3 f_normal;
in float f_alpha_test_mask;
in float f_discard_mask;

out vec4 color;

void main()
{
	vec4 c= texture( tex, f_tex_coord );

	// TODO - optimize discard operation here
	if( f_discard_mask < 0.5f || ( c.a < 0.5 && f_alpha_test_mask > 0.5 ) )
		discard;

	float normal_xy_square_length= dot( f_normal.xy, f_normal.xy );

	vec4 lightmap_value= texture( lightmap, f_lightmap_coord );
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
