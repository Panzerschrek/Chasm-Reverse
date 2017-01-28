uniform sampler2D tex;

in vec2 f_tex_coord;

out vec4 color;

void main()
{
	float l= texture( tex, f_tex_coord ).x;
	color= vec4( l, l, l, l );
}
