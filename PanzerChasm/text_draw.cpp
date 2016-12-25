#include <ogl_state_manager.hpp>
#include <shaders_loading.hpp>

#include "game_resources.hpp"
#include "images.hpp"
#include "vfs.hpp"

#include "text_draw.hpp"

namespace PanzerChasm
{

static const unsigned int g_max_letters_per_print= 2048u;

static const int g_letter_place_width= 16u;
static const int g_letter_place_height= 10u;
static const int g_letter_u_offset= 2u;
static const int g_letter_v_offset= 1u;
static const int g_letter_height= 9u;
static const int g_space_width= 6u;

static const unsigned int g_atlas_width = 16u * g_letter_place_width ;
static const unsigned int g_atlas_height= 16u * g_letter_place_height;

static const unsigned int g_colors_variations= 4u;

// Indeces of colors, used in font as inner for letters.
static const unsigned char g_start_inner_color= 4u;
static const unsigned char g_end_inner_color= 16u;

static const int g_color_shifts[ g_colors_variations ]=
{
	  0, // white
	 38, // dark-yellow
	176, // golden
	194, // dark-yellow with green
};

static const GLenum g_gl_state_blend_func[2]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
static const r_OGLState g_gl_state(
	false, false, false, false,
	g_gl_state_blend_func );

static void CalculateLettersWidth(
	const unsigned char* const texture_data,
	unsigned char* const out_width )
{
	for( unsigned int code= 0u; code < 256u; code++ )
	{
		const unsigned int tc_u= ( code & 15u ) * g_letter_place_width;
		const unsigned int tc_v= ( code >> 4u ) * g_letter_place_height;

		unsigned int max_x= 0u;
		for( unsigned int y= tc_v + g_letter_v_offset; y < tc_v + g_letter_height; y++ )
		{
			unsigned int max_line_x= tc_u + g_letter_u_offset;
			for( unsigned int x= tc_u + g_letter_u_offset; x < tc_u + g_letter_place_width; x++ )
			{
				if( texture_data[ x + y * g_atlas_width  ] != 255 )
					max_line_x= std::max( max_line_x, x );
			}
			max_x= std::max( max_x, max_line_x );
		}

		out_width[code]= 1u + max_x - tc_u - g_letter_u_offset;
	}

	out_width[' ']= g_space_width;
}

TextDraw::TextDraw(
	const RenderingContext& rendering_context,
	const GameResources& game_resources )
	: viewport_size_(rendering_context.viewport_size)
{
	// Texture
	const Vfs::FileContent font_file= game_resources.vfs->ReadFile( "FONT256.CEL" );
	const CelTextureHeader* const cel_header= reinterpret_cast<const CelTextureHeader*>( font_file.data() );
	const unsigned char* const fond_data= font_file.data() + sizeof(CelTextureHeader);

	const unsigned int pixel_count= cel_header->size[0] * cel_header->size[1];
	std::vector<unsigned char> font_rgba( g_colors_variations * 4u * pixel_count );

	std::vector<unsigned char> font_data_shifted( pixel_count );
	for( unsigned int c= 0u; c < g_colors_variations; c++ )
	{
		ColorShift(
			g_start_inner_color, g_end_inner_color,
			g_color_shifts[c],
			pixel_count,
			fond_data, font_data_shifted.data() );

		ConvertToRGBA(
			pixel_count,
			font_data_shifted.data(),
			game_resources.palette,
			font_rgba.data() + c * pixel_count * 4u );
	}

	CalculateLettersWidth(
		fond_data,
		letters_width_ );

	texture_=
		r_Texture(
			r_Texture::PixelFormat::RGBA8,
			cel_header->size[0],
			cel_header->size[1] * g_colors_variations,
			font_rgba.data() );

	texture_.SetFiltration(
		r_Texture::Filtration::Nearest,
		r_Texture::Filtration::Nearest );

	// Vertex buffer
	std::vector<unsigned short> indeces( 6u * g_max_letters_per_print );
	vertex_buffer_.resize( 4u * g_max_letters_per_print );

	for( unsigned int i= 0u; i < g_max_letters_per_print; i++ )
	{
		unsigned short* const ind= indeces.data() + 6u * i;
		ind[0]= 4u * i + 0u;  ind[1]= 4u * i + 1u;  ind[2]= 4u * i + 2u;
		ind[3]= 4u * i + 0u;  ind[4]= 4u * i + 2u;  ind[5]= 4u * i + 3u;
	}

	polygon_buffer_.VertexData(
		vertex_buffer_.data(),
		vertex_buffer_.size() * sizeof(Vertex),
		sizeof(Vertex) );

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

	// Shader

	shader_.ShaderSource(
		rLoadShader( "text_f.glsl", rendering_context.glsl_version ),
		rLoadShader( "text_v.glsl", rendering_context.glsl_version ) );
	shader_.SetAttribLocation( "pos", 0u );
	shader_.SetAttribLocation( "tex_coord", 1u );
	shader_.Create();
}

TextDraw::~TextDraw()
{}

unsigned int TextDraw::GetLineHeight() const
{
	return g_letter_height;
}

void TextDraw::Print(
	const int x, const int y,
	const char* text,
	const unsigned int scale,
	const FontColor color,
	const Alignment alignment )
{
	const int scale_i= int(scale);
	const int d_tc_v= int(color) * int(g_atlas_height);

	Vertex* v= vertex_buffer_.data();
	int current_x= x;
	int current_y= viewport_size_.Height() - y - scale * g_letter_height;

	Vertex* last_newline_vertex_index= v;
	while( 1 )
	{
		const unsigned int code= *text;
		if( code == '\n' || code == '\0' )
		{
			if( alignment == Alignment::Center )
			{
				const int center_x= ( last_newline_vertex_index->xy[0] + current_x ) / 2;
				const int delta_x= x - center_x;
				while( last_newline_vertex_index < v )
				{
					last_newline_vertex_index->xy[0]+= delta_x;
					last_newline_vertex_index++;
				}
			}

			current_x= x;
			current_y-= int(g_letter_height * scale);
			text++;

			if( code == '\0' ) break;
			else continue;
		}

		const unsigned int tc_u= ( code & 15u ) * g_letter_place_width  + g_letter_u_offset;
		const unsigned int tc_v= ( code >> 4u ) * g_letter_place_height + g_letter_v_offset + d_tc_v;
		const unsigned char letter_width= letters_width_[code];

		v[0].xy[0]= current_x;
		v[0].xy[1]= current_y;
		v[0].tex_coord[0]= tc_u;
		v[0].tex_coord[1]= tc_v + g_letter_height;

		v[1].xy[0]= current_x + letter_width * scale_i;
		v[1].xy[1]= current_y;
		v[1].tex_coord[0]= tc_u + letter_width;
		v[1].tex_coord[1]= tc_v + g_letter_height;

		v[2].xy[0]= current_x + letter_width  * scale_i;
		v[2].xy[1]= current_y + g_letter_height * scale_i;
		v[2].tex_coord[0]= tc_u + letter_width;
		v[2].tex_coord[1]= tc_v;

		v[3].xy[0]= current_x;
		v[3].xy[1]= current_y + g_letter_height * scale_i;
		v[3].tex_coord[0]= tc_u;
		v[3].tex_coord[1]= tc_v;

		current_x+= letter_width * scale_i;
		v+= 4u;
		text++;
	}

	const unsigned int vertex_count= v - vertex_buffer_.data();
	const unsigned int index_count= vertex_count / 4u * 6u;


	polygon_buffer_.VertexSubData(
		vertex_buffer_.data(),
		vertex_count * sizeof(Vertex),
		0u );

	// Draw
	r_OGLStateManager::UpdateState( g_gl_state );

	shader_.Bind();

	texture_.Bind(0u);
	shader_.Uniform( "tex", int(0) );

	shader_.Uniform(
		"inv_viewport_size",
		m_Vec2( 1.0f / float(viewport_size_.xy[0]), 1.0f / float(viewport_size_.xy[1]) ) );

	shader_.Uniform(
		"inv_texture_size",
		m_Vec2( 1.0f / float(texture_.Width()), 1.0f / float(texture_.Height()) ) );

	glDrawElements( GL_TRIANGLES, index_count, GL_UNSIGNED_SHORT, nullptr );
}

} // namespace PanzerChasm
