uniform mat4 view_matrix;

const vec2 coord[6]= vec2[6](
vec2( -1.0, -1.0 ), vec2( -1.0, +1.0 ), vec2( +1.0, +1.0 ),
vec2( -1.0, -1.0 ), vec2( +1.0, -1.0 ), vec2( +1.0, +1.0 ) );

const vec2 tex_coord[6]= vec2[6](
vec2( 0.0, 0.0 ), vec2( 0.0, 1.0 ), vec2( 1.0, 1.0 ),
vec2( 0.0, 0.0 ), vec2( 1.0, 0.0 ), vec2( 1.0, 1.0 ) );

out vec2 f_tex_coord;

void main()
{
	f_tex_coord= tex_coord[ gl_VertexID ];

	gl_Position= view_matrix * vec4( coord[ gl_VertexID ].x, 0.0, coord[ gl_VertexID ].y, 1.0 );
}
