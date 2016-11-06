uniform vec2 inv_viewport_size;
uniform vec2 inv_texture_size;

in vec2 pos;
in vec2 tex_coord;

noperspective out vec2 f_tex_coord;

void main()
{
	f_tex_coord= tex_coord * inv_texture_size;

	vec2 screen_pos= pos * inv_viewport_size * vec2( 2.0, 2.0 ) - vec2( 1.0, 1.0 );
	gl_Position= vec4( screen_pos, 0.0, 1.0 );
}
