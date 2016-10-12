uniform sampler2DArray tex;

in vec3 f_tex_coord;

out vec4 color;

void main()
{
	color= texture( tex, f_tex_coord );
}
