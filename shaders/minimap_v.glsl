uniform mat4 view_matrix;
uniform sampler1D visibility_texture;

in vec2 pos;

out float f_alpha;

void main()
{
	f_alpha= texelFetch( visibility_texture, gl_VertexID / 2, 0 ).x;

	gl_Position= view_matrix * vec4( pos, 0.0, 1.0 );
}
