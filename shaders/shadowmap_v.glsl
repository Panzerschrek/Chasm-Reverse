uniform mat4 view_matrix;

in vec2 pos;

void main()
{
	gl_Position= view_matrix * vec4( pos, 0.0, 1.0 );
}
