uniform sampler2DArray tex;
uniform float frame;
uniform float light;

in vec2 f_tex_coord;

out vec4 color;

void main()
{
	vec4 tex_value= texture( tex, vec3( f_tex_coord, frame ) );
	if( tex_value.a < 0.01 )
		discard;
	color= vec4( tex_value.xyz * light, tex_value.a * 0.5 );
}
