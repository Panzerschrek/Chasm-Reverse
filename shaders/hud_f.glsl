uniform sampler2D tex;

noperspective in vec2 f_tex_coord;

out vec4 color;

void main()
{
	vec4 tex_value= texture( tex, f_tex_coord );
	color= vec4( tex_value.xyz, tex_value.a * 0.5 );
}
