uniform sampler2D tex;

in vec3 f_pos;

out vec4 color;

//#define CONE_PROJECTION

void main()
{
	const float pi= 3.1415926535;
	const float half_pi= pi * 0.5;

	vec3 dir= normalize( f_pos );
	float xy= length( dir.xy );

#ifdef CONE_PROJECTION
	// Make cone projection
	// Alpha - angle of cone
	const float half_alpha= pi * 0.25;
	const float cos_half_alpha= cos( half_alpha );
	const float sin_half_alpha= sin( half_alpha );
	float k= 1.0 / ( 1.0 / tan( half_alpha ) + 1.0 / tan( pi * 0.25 - half_alpha * 0.5 ) );

	float g_tg_b=
		( cos_half_alpha * xy - sin_half_alpha * dir.z ) /
		( sin_half_alpha * xy + cos_half_alpha * dir.z );

	float tc_y= g_tg_b * k;
#else
	// Spherical projection.
	float tc_y= -asin( dir.z ) / half_pi;
#endif

	// take xy plane angle
	float tc_x= atan( dir.y, dir.x ) / ( 2.0 * pi );

	// Now, texture_coordinates is in rangle 0-1 for cone or sphere

	const vec2 scale= vec2( 5.0, 3.0 );

#if 0
	vec2 tc= scale * vec2( tc_x, tc_y );
	color= texture( tex, tc );
#else
	// Hack for mipmapping.
	// Make separate texture fetch on seam.
	vec2 tc0= scale * vec2( tc_x, tc_y );
	vec2 tc1= scale * vec2( tc_x + step( tc_x, 0.0 ), tc_y );
	color= mix( texture( tex, tc1 ), texture( tex, tc0 ), min( ( 0.5 - abs(tc_x) ) * 8.0, 1.0 ) );
#endif
}
