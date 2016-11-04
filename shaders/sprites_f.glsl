uniform sampler2DArray tex;
uniform float frame;

in vec2 f_tex_coord;

out vec4 color;

void main()
{
	vec4 tex_value= texture( tex, vec3( f_tex_coord, frame ) );
	color= vec4( tex_value.xyz, tex_value.a * 0.5 );
	//color= vec4( 1.0, 0.0, 1.0, 0.5 );
}
