uniform sampler2DArray tex;
uniform sampler2D lightmap;
//uniform float light= 1.0;

in vec3 f_tex_coord;
in vec2 f_lightmap_coord;

out vec4 color;

void main()
{
	float light= texture( lightmap, f_lightmap_coord ).x;
	color= vec4( light * texture( tex, f_tex_coord ).xyz, 0.25 );
}
