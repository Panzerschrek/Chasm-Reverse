uniform mat4 view_matrix;

in vec3 pos;
in vec2 tex_coord;
in int tex_id;

out vec3 f_tex_coord;

void main()
{
	f_tex_coord= vec3( tex_coord, float(tex_id) + 0.01 );
	gl_Position= view_matrix * vec4( pos, 1.0 );
}
