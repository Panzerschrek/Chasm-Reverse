uniform sampler2D ambient_lightmap;

in vec2 f_world_coord;
in vec2 f_normal;

out vec4 color;

void main()
{
	vec2 fetch_pos= f_world_coord + f_normal / 8.0; // TODO - calibrate shift
	color= texture( ambient_lightmap, fetch_pos / 64.0 );
}
