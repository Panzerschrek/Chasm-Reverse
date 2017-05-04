const vec2 coord[6]= vec2[6](
vec2( -1.0, -1.0 ), vec2( -1.0, +1.0 ), vec2( +1.0, +1.0 ),
vec2( -1.0, -1.0 ), vec2( +1.0, -1.0 ), vec2( +1.0, +1.0 ) );

void main()
{
	gl_Position= vec4( coord[ gl_VertexID ], 0.0, 1.0 );
}
