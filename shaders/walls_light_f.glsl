uniform sampler2D shadowmap;

uniform vec2 light_pos;
uniform float light_power;
uniform float max_light_level;
uniform float min_radius;
uniform float max_radius;

in vec2 f_world_coord;
in vec2 f_normal;

out vec4 color;

void main()
{
	vec2 r= light_pos - f_world_coord;
	float distance_to_light= distance( light_pos, f_world_coord );

	float light_fraction= 1.0 - min( max( distance_to_light - min_radius, 0.0 ) / ( max_radius - min_radius ), 1.0 );

	float shadow_factor= 1.0 - texture( shadowmap, f_world_coord / 64.0 ).x;

	vec2 normalized_dir_to_light= normalize( r );
	float normal_factor= max( 0, -dot( f_normal, normalized_dir_to_light ) );

	color=
		vec4(
			shadow_factor * normal_factor * min( light_power * light_fraction, max_light_level ),
			1.0, 1.0, 1.0 );
}
