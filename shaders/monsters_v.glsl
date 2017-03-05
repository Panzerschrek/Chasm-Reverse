#include "constants.glsl"

uniform mat4 view_matrix;
uniform mat3 lightmap_matrix;
uniform mat4 rotation_matrix;
uniform int enabled_groups_mask;

#ifdef USE_2D_TEXTURES_FOR_ANIMATIONS
uniform isampler2D animations_vertices_buffer;
#else
uniform isamplerBuffer animations_vertices_buffer;
#endif
uniform int first_animation_vertex_number;

in int vertex_id;
in vec2 tex_coord;
in float alpha_test_mask;
in int groups_mask;

out vec3 g_world_pos;
out vec2 g_tex_coord;
out vec2 g_lightmap_coord;
out float g_alpha_test_mask; // Polugon neeeds alpha-test
out float g_discard_mask; // Polygon discard

void main()
{
#ifdef USE_2D_TEXTURES_FOR_ANIMATIONS
	int final_vertex_id= first_animation_vertex_number + vertex_id;
	ivec2 animation_vertex_coord= ivec2( final_vertex_id & (ANIMATION_TEXTURE_WIDTH-1), final_vertex_id / ANIMATION_TEXTURE_WIDTH );
	vec3 pos=
		vec3( texelFetch(
			animations_vertices_buffer,
			animation_vertex_coord, 0 ).xyz ) * c_models_coordinates_scale;
#else
	vec3 pos=
		vec3( texelFetch(
			animations_vertices_buffer,
			first_animation_vertex_number + vertex_id ).xyz ) * c_models_coordinates_scale;
#endif

	g_world_pos= ( rotation_matrix * vec4( pos, 1.0 ) ).xyz;
	g_tex_coord= tex_coord;
	g_lightmap_coord= ( lightmap_matrix * vec3( pos.xy, 1.0 ) ).xy;
	g_alpha_test_mask= alpha_test_mask;
	g_discard_mask= ( enabled_groups_mask & groups_mask ) == 0 ? 0.0 : 1.0;
	gl_Position= view_matrix * vec4( pos, 1.0 );
}
