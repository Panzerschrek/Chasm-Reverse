uniform mat4 view_matrix;
uniform mat4 world_matrix;

const vec2 coord[6]= vec2[6](
vec2( -1.0, -1.0 ), vec2( -1.0, +1.0 ), vec2( +1.0, +1.0 ),
vec2( -1.0, -1.0 ), vec2( +1.0, -1.0 ), vec2( +1.0, +1.0 ) );

out vec2 f_world_coord;

void main()
{
	f_world_coord= ( world_matrix * vec4( coord[ gl_VertexID ], 0.0, 1.0 ) ).xy;
	gl_Position= view_matrix * vec4( coord[ gl_VertexID ].xy, 0.0, 1.0 );
}
