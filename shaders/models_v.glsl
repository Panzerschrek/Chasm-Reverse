uniform mat4 view_matrix;

in vec3 pos;
in vec2 tex_coord;

out vec2 f_tex_coord;

void main()
{
	f_tex_coord= tex_coord;
	gl_Position= view_matrix * vec4( pos, 1.0 );
}
