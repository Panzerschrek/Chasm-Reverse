#include <cstring>

#include <shaders_loading.hpp>

#include "menu_drawer.hpp"

namespace PanzerChasm
{

static const char* const g_menu_pictures[ size_t(MenuDrawer::MenuPicture::PicturesCount) ]=
{
	"M_MAIN.CEL",
	"M_NEW.CEL",
	"M_NETWRK.CEL",
};

static const unsigned int g_menu_pictures_shifts_count= 3u;
static const int g_menu_pictures_shifts[ g_menu_pictures_shifts_count ]=
{
	 0,
	64, // golden
	96, // dark-yellow
};

// Indeces of colors, used in ,enu pictures as inner for letters.
static const unsigned char g_start_inner_color= 7u * 16u;
static const unsigned char g_end_inner_color= 8u * 16u;

static const unsigned int g_menu_picture_row_height= 20u;
static const unsigned int g_menu_picture_horizontal_border= 4u;

static const unsigned int g_menu_border= 2u;
static const unsigned int g_menu_caption= 11u;

static const unsigned int g_max_quads= 512u;

MenuDrawer::MenuDrawer(
	const RenderingContext& rendering_context,
	const GameResources& game_resources )
	: viewport_size_(rendering_context.viewport_size)
{
	{ // Tiles texture
		const Vfs::FileContent tiles_texture_file= game_resources.vfs->ReadFile( "M_TILE1.CEL" );
		const CelTextureHeader* const cel_header=
			reinterpret_cast<const CelTextureHeader*>( tiles_texture_file.data() );

		const unsigned int pixel_count= cel_header->size[0] * cel_header->size[1];
		std::vector<unsigned char> tiles_texture_data_rgba( 4u * pixel_count );

		ConvertToRGBA(
			pixel_count,
			tiles_texture_file.data() + sizeof(CelTextureHeader),
			game_resources.palette,
			tiles_texture_data_rgba.data() );

		tiles_texture_=
			r_Texture(
				r_Texture::PixelFormat::RGBA8,
				cel_header->size[0],
				cel_header->size[1],
				tiles_texture_data_rgba.data() );

		tiles_texture_.SetFiltration(
			r_Texture::Filtration::Nearest,
			r_Texture::Filtration::Nearest );
	}
	{ // Menu pictures
		Vfs::FileContent picture_file;
		std::vector<unsigned char> picture_data_shifted;
		std::vector<unsigned char> picture_data_rgba;

		for( unsigned int i= 0u; i < size_t(MenuPicture::PicturesCount); i++ )
		{
			game_resources.vfs->ReadFile( g_menu_pictures[i], picture_file );

			const CelTextureHeader* const cel_header=
				reinterpret_cast<const CelTextureHeader*>( picture_file.data() );

			const unsigned int height_with_border= cel_header->size[1] + g_menu_picture_horizontal_border * 2u;

			const unsigned int in_pixel_count= cel_header->size[0] * cel_header->size[1];
			const unsigned int out_pixel_count= cel_header->size[0] * height_with_border;
			picture_data_rgba.resize( 4u * g_menu_pictures_shifts_count * out_pixel_count );
			picture_data_shifted.resize( in_pixel_count );

			for( unsigned int s= 0u; s < g_menu_pictures_shifts_count; s++ )
			{
				ColorShift(
					g_start_inner_color, g_end_inner_color,
					g_menu_pictures_shifts[ s ],
					in_pixel_count,
					picture_file.data() + sizeof(CelTextureHeader),
					picture_data_shifted.data() );

				const unsigned int border_size_bytes= 4u * g_menu_picture_horizontal_border * cel_header->size[0];

				ConvertToRGBA(
					in_pixel_count,
					picture_data_shifted.data(),
					game_resources.palette,
					picture_data_rgba.data() + 4u * s * out_pixel_count + border_size_bytes,
					0u );

				// Fill up and down borders
				std::memset(
					picture_data_rgba.data() + 4u * s * out_pixel_count,
					0,
					border_size_bytes );
				std::memset(
					picture_data_rgba.data() + 4u * ( s * out_pixel_count + in_pixel_count ) + border_size_bytes,
					0,
					border_size_bytes );
			}

			menu_pictures_[i]=
				r_Texture(
					r_Texture::PixelFormat::RGBA8,
					cel_header->size[0],
					height_with_border * g_menu_pictures_shifts_count,
					picture_data_rgba.data() );

			menu_pictures_[i].SetFiltration(
				r_Texture::Filtration::Nearest,
				r_Texture::Filtration::Nearest );
		}
	}

	// Polygon buffer
	std::vector<unsigned short> indeces( 6u * g_max_quads );
	for( unsigned int i= 0u; i < g_max_quads; i++ )
	{
		unsigned short* const ind= indeces.data() + 6u * i;
		ind[0]= 4u * i + 0u;  ind[1]= 4u * i + 1u;  ind[2]= 4u * i + 2u;
		ind[3]= 4u * i + 0u;  ind[4]= 4u * i + 2u;  ind[5]= 4u * i + 3u;
	}

	polygon_buffer_.VertexData( nullptr, 4u * g_max_quads * sizeof(Vertex), sizeof(Vertex) );
	polygon_buffer_.IndexData(
		indeces.data(),
		indeces.size() * sizeof(unsigned short),
		GL_UNSIGNED_SHORT,
		GL_TRIANGLES );

	Vertex v;
	polygon_buffer_.VertexAttribPointer(
		0,
		2, GL_SHORT, false,
		((char*)v.xy) - (char*)&v );
	polygon_buffer_.VertexAttribPointer(
		1,
		2, GL_SHORT, false,
		((char*)v.tex_coord) - (char*)&v );

	// Shaders

	menu_background_shader_.ShaderSource(
		rLoadShader( "menu_f.glsl", rendering_context.glsl_version ),
		rLoadShader( "menu_v.glsl", rendering_context.glsl_version ) );
	menu_background_shader_.SetAttribLocation( "pos", 0u );
	menu_background_shader_.SetAttribLocation( "tex_coord", 1u );
	menu_background_shader_.Create();

	menu_picture_shader_.ShaderSource(
		rLoadShader( "menu_picture_f.glsl", rendering_context.glsl_version ),
		rLoadShader( "menu_picture_v.glsl", rendering_context.glsl_version ) );
	menu_picture_shader_.SetAttribLocation( "pos", 0u );
	menu_picture_shader_.SetAttribLocation( "tex_coord", 1u );
	menu_picture_shader_.Create();
}

MenuDrawer::~MenuDrawer()
{
}

Size2 MenuDrawer::GetViewportSize() const
{
	return viewport_size_;
}

Size2 MenuDrawer::GetPictureSize( MenuPicture picture ) const
{
	const r_Texture& tex= menu_pictures_[ size_t(picture) ];
	return Size2( tex.Width(), tex.Height() / g_menu_pictures_shifts_count );
}

void MenuDrawer::DrawMenuBackground(
	const int x, const int y,
	const unsigned int width, const unsigned int height,
	const unsigned int scale )
{
	// Gen quad
	Vertex vertices[ g_max_quads * 4u ];

	const int scale_i= int(scale);

	vertices[0].xy[0]= x - g_menu_border * scale_i;
	vertices[0].xy[1]= y - g_menu_border * scale_i;

	vertices[1].xy[0]= x + width  + g_menu_border * scale_i;
	vertices[1].xy[1]= y - g_menu_border * scale_i;

	vertices[2].xy[0]= x + width  + g_menu_border * scale_i;
	vertices[2].xy[1]= y + height + ( g_menu_border + g_menu_caption ) * scale_i;

	vertices[3].xy[0]= x - g_menu_border * scale_i;
	vertices[3].xy[1]= vertices[2].xy[1];

	const unsigned int vertex_count= 4u;
	const unsigned int index_count= 6u;

	polygon_buffer_.VertexSubData(
		vertices,
		vertex_count * sizeof(Vertex),
		0u );

	// Draw
	menu_background_shader_.Bind();

	tiles_texture_.Bind(0u);
	menu_background_shader_.Uniform( "tex", int(0) );

	menu_background_shader_.Uniform(
		"inv_viewport_size",
		m_Vec2( 1.0f / float(viewport_size_.xy[0]), 1.0f / float(viewport_size_.xy[1]) ) );

	menu_background_shader_.Uniform(
		"inv_texture_size",
		 m_Vec2(
			1.0f / float(tiles_texture_.Width()),
			1.0f / float(tiles_texture_.Height()) ) / float(scale) );

	glDrawElements( GL_TRIANGLES, index_count, GL_UNSIGNED_SHORT, nullptr );
}

void MenuDrawer::DrawMenuPicture(
	const int x, const int y0,
	const MenuPicture pic,
	const PictureColor* const rows_colors,
	const unsigned int scale )
{
	const r_Texture& picture= menu_pictures_[ size_t(pic) ];

	// Gen quad
	Vertex vertices[ g_max_quads * 4u ];

	const int height= int( picture.Height() / g_menu_pictures_shifts_count );
	const int scale_i= int(scale);
	const int raw_height= int(g_menu_picture_row_height);

	const unsigned int row_count= picture.Height() / ( g_menu_picture_row_height * g_menu_pictures_shifts_count );

	for( unsigned int r= 0u; r < row_count; r++ )
	{
		Vertex* const v= vertices + r * 4u;
		const int y= y0 + int( ( row_count - 1u - r ) * g_menu_picture_row_height ) * scale_i;
		const int tc_y= int(g_menu_picture_row_height * r) + height * static_cast<int>(rows_colors[r]);

		v[0].xy[0]= x;
		v[0].xy[1]= y;
		v[0].tex_coord[0]= 0;
		v[0].tex_coord[1]= tc_y + raw_height;

		v[1].xy[0]= x + picture.Width() * scale_i;
		v[1].xy[1]= y;
		v[1].tex_coord[0]= picture.Width ();
		v[1].tex_coord[1]= tc_y + raw_height;

		v[2].xy[0]= x + picture.Width() * scale_i;
		v[2].xy[1]= y + raw_height * scale_i;
		v[2].tex_coord[0]= picture.Width();
		v[2].tex_coord[1]= tc_y;

		v[3].xy[0]= x;
		v[3].xy[1]= y + raw_height * scale_i;
		v[3].tex_coord[0]= 0;
		v[3].tex_coord[1]= tc_y;
	}

	const unsigned int vertex_count= row_count * 4u;
	const unsigned int index_count= row_count * 6u;

	polygon_buffer_.VertexSubData(
		vertices,
		vertex_count * sizeof(Vertex),
		0u );

	// Draw
	menu_picture_shader_.Bind();

	picture.Bind(0u);
	menu_picture_shader_.Uniform( "tex", int(0) );

	menu_picture_shader_.Uniform(
		"inv_viewport_size",
		m_Vec2( 1.0f / float(viewport_size_.xy[0]), 1.0f / float(viewport_size_.xy[1]) ) );

	menu_picture_shader_.Uniform(
		"inv_texture_size",
		 m_Vec2(
			1.0f / float(picture.Width()),
			1.0f / float(picture.Height()) ) );

	glDrawElements( GL_TRIANGLES, index_count, GL_UNSIGNED_SHORT, nullptr );
}

} // namespace PanzerChasm
