#include <cstring>

#include <ogl_state_manager.hpp>
#include <shaders_loading.hpp>

#include "game_resources.hpp"
#include "images.hpp"
#include "text_drawers_common.hpp"
#include "vfs.hpp"

#include "text_drawer_gl.hpp"

namespace PanzerChasm
{

const unsigned int g_max_letters_per_print= 2048u;

static const GLenum g_gl_state_blend_func[2]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };
static const r_OGLState g_gl_state(
	false, false, false, false,
	g_gl_state_blend_func );

TextDrawerGL::TextDrawerGL(
	const RenderingContextGL& rendering_context,
	const GameResources& game_resources )
	: viewport_size_(rendering_context.viewport_size)
{
	// Texture
	const Vfs::FileContent font_file= game_resources.vfs->ReadFile( FontParams::font_file_name );
	const CelTextureHeader* const cel_header= reinterpret_cast<const CelTextureHeader*>( font_file.data() );
	const unsigned char* const font_data= font_file.data() + sizeof(CelTextureHeader);

	const unsigned int pixel_count= cel_header->size[0] * cel_header->size[1];
	std::vector<unsigned char> font_rgba( FontParams::colors_variations * 4u * pixel_count );

	std::vector<unsigned char> font_data_shifted( pixel_count );
	for( unsigned int c= 0u; c < FontParams::colors_variations; c++ )
	{
		ColorShift(
			FontParams::start_inner_color, FontParams::end_inner_color,
			FontParams::color_shifts[c],
			pixel_count,
			font_data, font_data_shifted.data() );

		// Hack. Move "slider" letter from code '\0';
		for( unsigned int y= 0; y < FontParams::letter_place_height; y++ )
			std::memcpy(
				font_data_shifted.data() + FontParams::atlas_width * y + FontParams::letter_place_width * ( c_slider_back_letter_code & 15u ) +  FontParams::atlas_width * FontParams::letter_place_height * ( c_slider_back_letter_code >> 4u ),
				font_data_shifted.data() + FontParams::atlas_width * y,
				FontParams::letter_place_width );

		ConvertToRGBA(
			pixel_count,
			font_data_shifted.data(),
			game_resources.palette,
			font_rgba.data() + c * pixel_count * 4u );

	}

	CalculateLettersWidth(
		font_data,
		letters_width_ );

	letters_width_[ c_slider_back_letter_code ]= letters_width_[0]; // Move slider letter.

	texture_=
		r_Texture(
			r_Texture::PixelFormat::RGBA8,
			cel_header->size[0],
			cel_header->size[1] * FontParams::colors_variations,
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

TextDrawerGL::~TextDrawerGL()
{}

unsigned int TextDrawerGL::GetLineHeight() const
{
	return FontParams::letter_height;
}

void TextDrawerGL::Print(
	const int x, const int y,
	const char* text,
	const unsigned int scale,
	const FontColor color,
	const Alignment alignment )
{
	const int scale_i= int(scale);
	const int d_tc_v= int(color) * int(FontParams::atlas_height);

	Vertex* v= vertex_buffer_.data();
	int current_x= x;
	int current_y= viewport_size_.Height() - y - scale * FontParams::letter_height;

	Vertex* last_newline_vertex_index= v;
	while( 1 )
	{
		const unsigned int code= static_cast<unsigned char>(*text); // Convert to unsigned - allow chars with codes 128 - 255.
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
			else if( alignment == Alignment::Right )
			{
				const int delta_x= last_newline_vertex_index->xy[0] - current_x;
				while( last_newline_vertex_index < v )
				{
					last_newline_vertex_index->xy[0]+= delta_x;
					last_newline_vertex_index++;
				}
			}

			current_x= x;
			current_y-= int(FontParams::letter_height * scale);
			text++;

			if( code == '\0' ) break;
			else continue;
		}

		const unsigned int tc_u= ( code & 15u ) * FontParams::letter_place_width  + FontParams::letter_u_offset;
		const unsigned int tc_v= ( code >> 4u ) * FontParams::letter_place_height + FontParams::letter_v_offset + d_tc_v;
		const unsigned char letter_width= letters_width_[code];

		v[0].xy[0]= current_x;
		v[0].xy[1]= current_y;
		v[0].tex_coord[0]= tc_u;
		v[0].tex_coord[1]= tc_v + FontParams::letter_height;

		v[1].xy[0]= current_x + letter_width * scale_i;
		v[1].xy[1]= current_y;
		v[1].tex_coord[0]= tc_u + letter_width;
		v[1].tex_coord[1]= tc_v + FontParams::letter_height;

		v[2].xy[0]= current_x + letter_width  * scale_i;
		v[2].xy[1]= current_y + FontParams::letter_height * scale_i;
		v[2].tex_coord[0]= tc_u + letter_width;
		v[2].tex_coord[1]= tc_v;

		v[3].xy[0]= current_x;
		v[3].xy[1]= current_y + FontParams::letter_height * scale_i;
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
