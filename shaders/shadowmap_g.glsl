layout( lines ) in;
layout( triangle_strip, max_vertices = 6 ) out;

uniform vec2 light_pos;
uniform float offset;

void main()
{
	vec2 dir0= gl_in[0].gl_Position.xy - light_pos;
	dir0= normalize(dir0);

	vec2 dir1= gl_in[1].gl_Position.xy - light_pos;
	dir1= normalize(dir1);

	// TODO - do not make such big polygons. We need do it with light radius only.
	float len= 1.0;

	vec4 v0= gl_in[0].gl_Position + vec4( dir0 * offset, 0.0, 0.0 );
	vec4 v1= gl_in[1].gl_Position + vec4( dir1 * offset, 0.0, 0.0 );
	vec4 v2= gl_in[0].gl_Position + vec4( dir0 * len, 0.0, 0.0 );
	vec4 v3= gl_in[1].gl_Position + vec4( dir1 * len, 0.0, 0.0 );

	// TODO - emit more polygons for soft shadows.

	gl_Position= v0;
	EmitVertex();

	gl_Position= v1;
	EmitVertex();

	gl_Position= v2;
	EmitVertex();
	EndPrimitive();

	gl_Position= v1;
	EmitVertex();

	gl_Position= v3;
	EmitVertex();

	gl_Position= v2;
	EmitVertex();
	EndPrimitive();
}
