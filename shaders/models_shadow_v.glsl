#include "constants.glsl"

uniform mat4 view_matrix;
uniform int enabled_groups_mask;
uniform vec3 light_pos; // Model space light position.

#ifdef USE_2D_TEXTURES_FOR_ANIMATIONS
uniform isampler2D animations_vertices_buffer;
#else
uniform isamplerBuffer animations_vertices_buffer;
#endif
uniform int first_animation_vertex_number;

in int vertex_id;
in int groups_mask;

out float f_discard_mask; // Polygon discard

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

	f_discard_mask= ( enabled_groups_mask & groups_mask ) == 0 ? 0.0 : 1.0;

	vec3 vec_from_light= pos - light_pos;
	vec3 pos_projected= light_pos + vec_from_light / vec_from_light.z * (-light_pos.z);
	gl_Position= view_matrix * vec4( pos_projected.xy, c_shadows_z_offset, 1.0 );
}
