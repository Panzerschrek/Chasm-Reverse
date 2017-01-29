uniform sampler2DArray tex;
uniform sampler2D lightmap;

in vec3 f_tex_coord;
in vec2 f_lightmap_coord;

out vec4 color;

void main()
{
	float light= texture( lightmap, f_lightmap_coord ).x;
	vec4 c= texture( tex, f_tex_coord );

	if( c.a < 0.5 )
		discard;

	color= c * light;
}
