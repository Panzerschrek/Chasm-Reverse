uniform mat4 view_matrix;
uniform float pos_z;

in vec2 pos;
in int tex_id;

out vec3 f_tex_coord;
out vec2 f_lightmap_coord;

void main()
{
	f_tex_coord= vec3( pos, float(tex_id) + 0.01 );
	f_lightmap_coord= pos * LIGHTMAP_SCALE;

	gl_Position= view_matrix * vec4( pos, pos_z, 1.0f );
}
