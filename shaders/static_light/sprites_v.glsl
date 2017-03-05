#include "constants.glsl"

uniform mat4 view_matrix;
uniform vec2 lightmap_coord;
uniform sampler2D lightmap;

const vec2 coord[6]= vec2[6](
vec2( -1.0, -1.0 ), vec2( -1.0, +1.0 ), vec2( +1.0, +1.0 ),
vec2( -1.0, -1.0 ), vec2( +1.0, -1.0 ), vec2( +1.0, +1.0 ) );

const vec2 tex_coord[6]= vec2[6](
vec2( 0.0, 0.0 ), vec2( 0.0, 1.0 ), vec2( 1.0, 1.0 ),
vec2( 0.0, 0.0 ), vec2( 1.0, 0.0 ), vec2( 1.0, 1.0 ) );

out vec2 f_tex_coord;
out float f_light;

void main()
{
	f_tex_coord= tex_coord[ gl_VertexID ];
	f_light= c_static_light_scale * texture( lightmap, lightmap_coord ).x;

	gl_Position= view_matrix * vec4( coord[ gl_VertexID ].x, 0.0, coord[ gl_VertexID ].y, 1.0 );
}
