uniform mat4 view_matrix;

in vec3 pos;
in vec2 tex_coord;
in int tex_id;
in vec2 normal;
in vec2 lightmap_coord;

out vec3 f_tex_coord;
out vec2 f_lightmap_coord;

void main()
{
	f_tex_coord= vec3( tex_coord / 256.0, float( tex_id ) + 0.01 );
	//f_lightmap_coord= ( pos.xy / 256.0 + normal / 8.0 ) * LIGHTMAP_SCALE;
	f_lightmap_coord= vec2( lightmap_coord.x, lightmap_coord.y + 0.5 ) / 64.0;

	gl_Position= view_matrix * vec4( pos, 1.0f );
}
