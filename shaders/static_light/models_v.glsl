uniform mat4 view_matrix;
uniform mat3 lightmap_matrix;

uniform isamplerBuffer animations_vertices_buffer;
uniform int first_animation_vertex_number;

in int vertex_id;
in vec2 tex_coord;
in int tex_id;
in float alpha_test_mask;

out vec3 f_tex_coord;
out vec2 f_lightmap_coord;
out float f_alpha_test_mask;

void main()
{
	vec3 pos=
		vec3( texelFetch(
			animations_vertices_buffer,
			first_animation_vertex_number + vertex_id ).xyz ) / 2048.0;

	f_tex_coord= vec3( tex_coord, float(tex_id) + 0.01 );
	f_lightmap_coord= ( lightmap_matrix * vec3( pos.xy, 1.0 ) ).xy;
	f_alpha_test_mask= alpha_test_mask;
	gl_Position= view_matrix * vec4( pos, 1.0 );
}
