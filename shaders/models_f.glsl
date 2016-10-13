uniform sampler2DArray tex;

in vec3 f_tex_coord;

out vec4 color;

void main()
{
	color= vec4( texture( tex, f_tex_coord ).xyz, 0.25 );
}
