uniform sampler2D tex;

in vec3 f_tex_coord;

out vec4 color;

void main()
{
	//color= vec4( 0.0, 1.0, 0.0, 0.0 );
	color= vec4( texture( tex, f_tex_coord.xy ).xyz, 0.25 );
}
