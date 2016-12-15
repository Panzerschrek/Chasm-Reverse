uniform mat4 view_matrix;

in vec3 pos;

out vec3 f_pos;

void main()
{
	f_pos= pos;
	gl_Position= view_matrix * vec4( pos, 1.0 );
}
