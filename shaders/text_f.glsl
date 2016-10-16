uniform sampler2D tex;

noperspective in vec2 f_tex_coord;

out vec4 color;

void main()
{
	vec4 c= texture( tex, f_tex_coord );
	//if( c.a < 0.5 )
	//	discard;

	c= mix( c, vec4( 1.0, 0.0, 1.0, 0.5 ), step( c.a, 0.5 ) );

	color= c;
}
