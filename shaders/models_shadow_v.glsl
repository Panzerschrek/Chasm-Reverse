#include "constants.glsl"

uniform mat4 view_matrix;

#ifdef USE_2D_TEXTURES_FOR_ANIMATIONS
uniform isampler2D animations_vertices_buffer;
#else
uniform isamplerBuffer animations_vertices_buffer;
#endif
uniform int first_animation_vertex_number;

in int vertex_id;

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

	gl_Position= view_matrix * vec4( pos.xy, 0.01, 1.0 );
}
