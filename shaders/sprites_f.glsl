uniform sampler2DArray tex;
uniform float frame;

in vec2 f_tex_coord;
in float f_light;

out vec4 color;

void main()
{
	vec4 tex_value= texture( tex, vec3( f_tex_coord, frame ) );
	if( tex_value.a < 0.01 )
		discard;
	color= vec4( tex_value.xyz * f_light, tex_value.a * 0.4 );
}
