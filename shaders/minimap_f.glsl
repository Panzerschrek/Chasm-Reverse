uniform vec4 color;

in float f_alpha;

out vec4 out_color;

void main()
{
	out_color= vec4( color.xyz, color.a * f_alpha );
}
