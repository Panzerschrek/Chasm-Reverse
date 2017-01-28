layout( triangles ) in;
layout( triangle_strip, max_vertices = 3 ) out;

in vec3 g_world_pos[];
in vec3 g_tex_coord[];
in vec2 g_lightmap_coord[];
in float g_alpha_test_mask[];

out vec3 f_tex_coord;
out vec3 f_normal;
out vec2 f_lightmap_coord;
out float f_alpha_test_mask;


void main()
{
	vec3 normal= cross( g_world_pos[0] - g_world_pos[1], g_world_pos[1] - g_world_pos[2] );
	normal= normalize( normal );

	gl_Position= gl_in[0].gl_Position;
	f_normal= normal;
	f_tex_coord= g_tex_coord[0];
	f_lightmap_coord= g_lightmap_coord[0];
	f_alpha_test_mask= g_alpha_test_mask[0];
	EmitVertex();

	gl_Position= gl_in[1].gl_Position;
	f_normal= normal;
	f_tex_coord= g_tex_coord[1];
	f_lightmap_coord= g_lightmap_coord[1];
	f_alpha_test_mask= g_alpha_test_mask[1];
	EmitVertex();

	gl_Position= gl_in[2].gl_Position;
	f_normal= normal;
	f_tex_coord= g_tex_coord[2];
	f_lightmap_coord= g_lightmap_coord[2];
	f_alpha_test_mask= g_alpha_test_mask[2];
	EmitVertex();

	EndPrimitive();
}
