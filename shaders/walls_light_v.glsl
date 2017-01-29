in vec2 pos;
in vec2 lightmap_coord;
in vec2 normal;

out vec2 f_world_coord;
out vec2 f_normal;

void main()
{
	f_world_coord= pos / 256.0;
	f_normal= normal;

	vec2 pos= lightmap_coord;
	pos.y+= 0.5;

	gl_Position= vec4( pos / 64.0 * 2.0 - vec2( 1.0, 1.0 ), 0.0, 1.0 );
}
