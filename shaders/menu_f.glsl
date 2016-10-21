uniform sampler2D tile_tex;
uniform sampler2D framing_tex;

// xy - tile, zw - framing
noperspective in vec4 f_tex_coord;

out vec4 color;

void main()
{
	color= 1.5 *
		texture( tile_tex, f_tex_coord.xy ) *
		texture( framing_tex, f_tex_coord.zw ).x;
}
