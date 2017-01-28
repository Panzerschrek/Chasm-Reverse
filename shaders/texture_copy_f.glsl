uniform sampler2D tex;

in vec2 f_tex_coord;

out vec4 color;

void main()
{
	color= texture( tex, f_tex_coord );
}
