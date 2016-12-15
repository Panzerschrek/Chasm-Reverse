uniform sampler2D tex;

in vec3 f_pos;

out vec4 color;

void main()
{
	vec2 tc= f_pos.xy / ( abs( f_pos.z ) + 0.05 );

	color= texture( tex, tc / 4.0 );
}
