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

static const unsigned int g_max_quads= 512u;

MenuDrawer::MenuDrawer(
	const unsigned int viewport_width ,
	const unsigned int viewport_height,
	const GameResources& game_resources )
{
	viewport_size_[0]= viewport_width ;
	viewport_size_[1]= viewport_height;

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

			const unsigned int pixel_count= cel_header->size[0] * cel_header->size[1];
			picture_data_rgba.resize( 4u * g_menu_pictures_shifts_count * pixel_count );
			picture_data_shifted.resize( pixel_count );

			for( unsigned int s= 0u; s < g_menu_pictures_shifts_count; s++ )
			{
				ColorShift(
					g_start_inner_color, g_end_inner_color,
					g_menu_pictures_shifts[ s ],
					pixel_count,
					picture_file.data() + sizeof(CelTextureHeader),
					picture_data_shifted.data() );

				ConvertToRGBA(
					pixel_count,
					picture_data_shifted.data(),
					game_resources.palette,
					picture_data_rgba.data() + 4u * s * pixel_count );
			}

			menu_pictures_[i]=
				r_Texture(
					r_Texture::PixelFormat::RGBA8,
					cel_header->size[0],
					cel_header->size[1] * g_menu_pictures_shifts_count,
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

	// Shader
	const r_GLSLVersion glsl_version( r_GLSLVersion::KnowmNumbers::v330, r_GLSLVersion::Profile::Core );

	shader_.ShaderSource(
		rLoadShader( "menu_f.glsl", glsl_version ),
		rLoadShader( "menu_v.glsl", glsl_version ) );
	shader_.SetAttribLocation( "pos", 0u );
	shader_.Create();
}

MenuDrawer::~MenuDrawer()
{
}

void MenuDrawer::DrawMenuBackground(
	const unsigned int width, const unsigned int height,
	const unsigned int texture_scale )
{
	// Gen quad
	Vertex vertices[ g_max_quads * 4u ];

	const int x= int( ( viewport_size_[0] - width ) >> 1u );
	const int y= ( int(viewport_size_[1]) - int(height) ) >> 1;

	vertices[0].xy[0]= x;
	vertices[0].xy[1]= y;

	vertices[1].xy[0]= x + width ;
	vertices[1].xy[1]= y;

	vertices[2].xy[0]= x + width ;
	vertices[2].xy[1]= y + height;

	vertices[3].xy[0]= x;
	vertices[3].xy[1]= y + height;

	const unsigned int vertex_count= 4u;
	const unsigned int index_count= 6u;

	polygon_buffer_.VertexSubData(
		vertices,
		vertex_count * sizeof(Vertex),
		0u );

	// Draw
	shader_.Bind();

	tiles_texture_.Bind(0u);
	shader_.Uniform( "tex", int(0) );

	shader_.Uniform(
		"inv_viewport_size",
		m_Vec2( 1.0f / float(viewport_size_[0]), 1.0f / float(viewport_size_[1]) ) );

	shader_.Uniform(
		"inv_texture_size",
		 m_Vec2(
			1.0f / float(tiles_texture_.Width()),
			1.0f / float(tiles_texture_.Height()) ) / float(texture_scale) );

	glDrawElements( GL_TRIANGLES, index_count, GL_UNSIGNED_SHORT, nullptr );
}

} // namespace PanzerChasm
