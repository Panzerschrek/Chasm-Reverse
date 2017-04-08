#include <ogl_state_manager.hpp>
#include <shaders_loading.hpp>

#include "../assert.hpp"
#include "../map_loader.hpp"
#include "map_state.hpp"
#include "minimap_drawers_common.hpp"

#include "minimap_drawer_gl.hpp"

namespace PanzerChasm
{

static const GLenum g_gl_state_blend_func[2]= { GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA };

static const r_OGLState g_gl_state(
	true, false, false, false,
	g_gl_state_blend_func );

static const unsigned int g_arrow_lines= 3u;
static const unsigned int g_arrow_vertices= g_arrow_lines * 2u;
static const unsigned int g_framing_lines= 4u;
static const unsigned int g_framing_vertices= g_framing_lines * 2u;

MinimapDrawerGL::MinimapDrawerGL(
	Settings& settings,
	const GameResourcesConstPtr& game_resources,
	const RenderingContextGL& rendering_context )
	: game_resources_(game_resources)
	, rendering_context_(rendering_context)
{
	PC_UNUSED( settings );
	PC_ASSERT( game_resources != nullptr );

	glGenTextures( 1, &visibility_texture_id_ );
	glGenTextures( 1, &dummy_visibility_texture_id_ );

	lines_shader_.ShaderSource(
		rLoadShader( "minimap_f.glsl", rendering_context.glsl_version ),
		rLoadShader( "minimap_v.glsl", rendering_context.glsl_version ) );
	lines_shader_.SetAttribLocation( "pos", 0 );
	lines_shader_.Create();
}

MinimapDrawerGL::~MinimapDrawerGL()
{
	glDeleteTextures( 1, &visibility_texture_id_ );
	glDeleteTextures( 1, &dummy_visibility_texture_id_ );
}

void MinimapDrawerGL::SetMap( const MapDataConstPtr& map_data )
{
	if( current_map_data_ == map_data )
		return;

	current_map_data_= map_data;
	if( map_data == nullptr )
		return;

	const unsigned int line_count=
		current_map_data_->static_walls.size() +
		current_map_data_->dynamic_walls.size() +
		g_arrow_lines +
		g_framing_lines;

	std::vector<WallLineVertex> vertices( line_count * 2u );

	first_static_walls_vertex_= 0u * 2u;
	first_dynamic_walls_vertex_= current_map_data_->static_walls.size() * 2u;
	arrow_vertices_offset_= first_dynamic_walls_vertex_ + current_map_data_->dynamic_walls.size() * 2u;
	framing_vertices_offset_= arrow_vertices_offset_ + g_arrow_vertices;

	for( unsigned int w= 0u; w < current_map_data_->static_walls.size(); w++ )
	{
		const MapData::Wall& in_wall= current_map_data_->static_walls[w];
		vertices[ first_static_walls_vertex_ + w * 2u + 0u ]= in_wall.vert_pos[0];
		vertices[ first_static_walls_vertex_ + w * 2u + 1u ]= in_wall.vert_pos[1];
	}

	for( unsigned int w= 0u; w < current_map_data_->dynamic_walls.size(); w++ )
	{
		const MapData::Wall& in_wall= current_map_data_->dynamic_walls[w];
		vertices[ first_dynamic_walls_vertex_ + w * 2u + 0u ]= in_wall.vert_pos[0];
		vertices[ first_dynamic_walls_vertex_ + w * 2u + 1u ]= in_wall.vert_pos[1];
	}

	// Arrow
	vertices[ arrow_vertices_offset_ + 0u + 0u ].x= -0.5f;
	vertices[ arrow_vertices_offset_ + 0u + 0u ].y= -1.0f;
	vertices[ arrow_vertices_offset_ + 0u + 1u ].x= +0.5f;
	vertices[ arrow_vertices_offset_ + 0u + 1u ].y= -1.0f;

	vertices[ arrow_vertices_offset_ + 2u + 0u ].x= -0.5f;
	vertices[ arrow_vertices_offset_ + 2u + 0u ].y= -1.0f;
	vertices[ arrow_vertices_offset_ + 2u + 1u ].x= +0.0f;
	vertices[ arrow_vertices_offset_ + 2u + 1u ].y= +1.0f;

	vertices[ arrow_vertices_offset_ + 4u + 0u ].x= +0.5f;
	vertices[ arrow_vertices_offset_ + 4u + 0u ].y= -1.0f;
	vertices[ arrow_vertices_offset_ + 4u + 1u ].x= +0.0f;
	vertices[ arrow_vertices_offset_ + 4u + 1u ].y= +1.0f;

	// Framing
	// Bottom
	vertices[ framing_vertices_offset_ + 0u + 0u ].x= -1.0f;
	vertices[ framing_vertices_offset_ + 0u + 0u ].y= -1.0f;
	vertices[ framing_vertices_offset_ + 0u + 1u ].x= +1.0f;
	vertices[ framing_vertices_offset_ + 0u + 1u ].y= -1.0f;
	// Left
	vertices[ framing_vertices_offset_ + 2u + 0u ].x= -1.0f;
	vertices[ framing_vertices_offset_ + 2u + 0u ].y= -1.0f;
	vertices[ framing_vertices_offset_ + 2u + 1u ].x= -1.0f;
	vertices[ framing_vertices_offset_ + 2u + 1u ].y= +1.0f;
	// Right
	vertices[ framing_vertices_offset_ + 4u + 0u ].x= +1.0f;
	vertices[ framing_vertices_offset_ + 4u + 0u ].y= -1.0f;
	vertices[ framing_vertices_offset_ + 4u + 1u ].x= +1.0f;
	vertices[ framing_vertices_offset_ + 4u + 1u ].y= +1.0f;
	// Top
	vertices[ framing_vertices_offset_ + 6u + 0u ].x= -1.0f;
	vertices[ framing_vertices_offset_ + 6u + 0u ].y= +1.0f;
	vertices[ framing_vertices_offset_ + 6u + 1u ].x= +1.0f;
	vertices[ framing_vertices_offset_ + 6u + 1u ].y= +1.0f;

	// Move to GPU
	walls_buffer_.VertexData( vertices.data(), vertices.size() * sizeof(WallLineVertex), sizeof(WallLineVertex) );
	walls_buffer_.VertexAttribPointer( 0, 2, GL_FLOAT, false, 0 );
	walls_buffer_.SetPrimitiveType( GL_LINES );

	// Prepare visibility texture.
	visibility_texture_data_.resize( line_count );
	glBindTexture( GL_TEXTURE_1D, visibility_texture_id_ );
	glTexImage1D( GL_TEXTURE_1D, 0, GL_R8, line_count, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr );
	glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

	// Prepare dummy visibility texture.
	// This is texture with all lines visible.
	for( unsigned char& c : visibility_texture_data_ ) c= 255u;

	glBindTexture( GL_TEXTURE_1D, dummy_visibility_texture_id_ );
	glTexImage1D( GL_TEXTURE_1D, 0, GL_R8, line_count, 0, GL_RED, GL_UNSIGNED_BYTE, visibility_texture_data_.data() );
	glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
}

void MinimapDrawerGL::Draw(
	const MapState& map_state, const MinimapState& minimap_state,
	const m_Vec2& camera_position, const float view_angle )
{
	if( current_map_data_ == nullptr )
		return;

	UpdateDynamicWalls( map_state );
	UpdateWallsVisibility( minimap_state );


	const float scale= static_cast<float>( CalculateMinimapScale( rendering_context_.viewport_size ) );

	const unsigned int top= static_cast<unsigned int>( MinimapParams::top_offset_px * scale );
	const unsigned int left= static_cast<unsigned int>( MinimapParams::left_offset_px * scale );
	const unsigned int right= rendering_context_.viewport_size.Width() / 2u - static_cast<unsigned int>( MinimapParams::right_offset_from_center_px * scale );
	const unsigned int bottom= static_cast<unsigned int>( ( 1.0f - MinimapParams::bottom_offset_rel ) * static_cast<float>( rendering_context_.viewport_size.Height() ) );
	const unsigned int map_viewport_width = right - left;
	const unsigned int map_viewport_height= bottom - top;

	r_OGLStateManager::UpdateState( g_gl_state );

	lines_shader_.Bind();

	m_Mat4 shift_matrix, scale_matrix, viewport_scale_matrix, viewport_shift_matrix;

	shift_matrix.Translate( m_Vec3( -camera_position, 0.0f ) );

	const float c_base_scale= -1.0f / 12.0f;
	const float aspect= float(rendering_context_.viewport_size.Height()) / float(rendering_context_.viewport_size.Width());
	scale_matrix.Scale( m_Vec3( aspect * c_base_scale, c_base_scale, 1.0f ) );

	const float map_viewport_scale= float(map_viewport_height) / float(rendering_context_.viewport_size.Height());
	viewport_scale_matrix.Scale( m_Vec3( map_viewport_scale, map_viewport_scale, 1.0f ) );

	viewport_shift_matrix.Translate(
		m_Vec3(
			float( int(right + left) - int(rendering_context_.viewport_size.Width ()) ) / float(rendering_context_.viewport_size.Width()),
			float( int(rendering_context_.viewport_size.Height()) - bottom - top ) / float(rendering_context_.viewport_size.Height()),
			0.0f ) );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_1D, visibility_texture_id_ );
	lines_shader_.Uniform( "visibility_texture", 0 );

	walls_buffer_.Bind();

	glLineWidth( scale );

	// Walls
	BindColor( MinimapParams::walls_color );
	lines_shader_.Uniform( "view_matrix", shift_matrix * scale_matrix * viewport_scale_matrix * viewport_shift_matrix );

	glEnable( GL_SCISSOR_TEST );
	glScissor( left, rendering_context_.viewport_size.Height() - bottom, map_viewport_width, map_viewport_height );
	glDrawArrays( GL_LINES, 0, ( current_map_data_->static_walls.size() + current_map_data_->dynamic_walls.size() ) * 2u );
	glDisable( GL_SCISSOR_TEST );

	// Draw arrow and framing with differnet visibility texture with all lines visible.
	// This needs, because glDrawArrays with first!= 0 generates different gl_VertexID in different OpenGL realizations.
	// In some realizations, gl_VertexID starts with 0? even if first !=0.
	// In this realizations reading of visibility for arrow/framing works incorrect.
	glBindTexture( GL_TEXTURE_1D, dummy_visibility_texture_id_ );

	// Arrow
	{
		m_Mat4 arrow_scale_matrix, rotate_matrix;
		const float c_arrow_scale= 0.5f;
		arrow_scale_matrix.Scale( m_Vec3( c_arrow_scale, c_arrow_scale, 1.0f ) );
		rotate_matrix.RotateZ( view_angle );

		BindColor( MinimapParams::arrow_color );
		lines_shader_.Uniform( "view_matrix", arrow_scale_matrix * rotate_matrix * scale_matrix * viewport_scale_matrix * viewport_shift_matrix );

		glDrawArrays( GL_LINES, arrow_vertices_offset_, g_arrow_vertices );
	}
	// Framing
	{
		m_Mat4 framing_scale_mat;
		framing_scale_mat.Scale(
			m_Vec3(
				float(map_viewport_width ) / float(rendering_context_.viewport_size.Width () ),
				float(map_viewport_height) / float(rendering_context_.viewport_size.Height() ),
				1.0f ) );

		lines_shader_.Uniform( "view_matrix", framing_scale_mat * viewport_shift_matrix );

		BindColor( MinimapParams::framing_contour_color );
		glLineWidth( scale * 3u );
		glDrawArrays( GL_LINES, framing_vertices_offset_, g_framing_vertices );

		BindColor( MinimapParams::framing_color );
		glLineWidth( scale );
		glDrawArrays( GL_LINES, framing_vertices_offset_, g_framing_vertices );
	}

	glLineWidth( 1.0f );
}

void MinimapDrawerGL::BindColor( const unsigned char color_index )
{
	const Palette& palette= game_resources_->palette;

	lines_shader_.Uniform(
		"color",
		float( palette[ color_index * 3u + 0u ] ) / 255.0f,
		float( palette[ color_index * 3u + 1u ] ) / 255.0f,
		float( palette[ color_index * 3u + 2u ] ) / 255.0f,
		0.7f );
}

void MinimapDrawerGL::UpdateDynamicWalls( const MapState& map_state )
{
	const MapState::DynamicWalls& dynamic_walls= map_state.GetDynamicWalls();

	dynamic_walls_vertices_.resize( dynamic_walls.size() * 2u );

	for( unsigned int w= 0u; w < dynamic_walls.size(); w++ )
	{
		dynamic_walls_vertices_[ w * 2u + 0u ]= dynamic_walls[w].vert_pos[0];
		dynamic_walls_vertices_[ w * 2u + 1u ]= dynamic_walls[w].vert_pos[1];
	}

	walls_buffer_.VertexSubData(
		dynamic_walls_vertices_.data(),
		dynamic_walls_vertices_.size() * sizeof(WallLineVertex),
		first_dynamic_walls_vertex_ * sizeof(WallLineVertex) );
}

void MinimapDrawerGL::UpdateWallsVisibility( const MinimapState& minimap_state )
{
	const MinimapState::WallsVisibility& static_walls_visibility = minimap_state.GetStaticWallsVisibility ();
	const MinimapState::WallsVisibility& dynamic_walls_visibility= minimap_state.GetDynamicWallsVisibility();

	PC_ASSERT( static_walls_visibility .size() == current_map_data_->static_walls .size() );
	PC_ASSERT( dynamic_walls_visibility.size() == current_map_data_->dynamic_walls.size() );

	for( unsigned int w= 0u; w < static_walls_visibility .size(); w++ )
		visibility_texture_data_[ first_static_walls_vertex_  / 2u + w ]= static_walls_visibility [w] ? 255u : 0u;
	for( unsigned int w= 0u; w < dynamic_walls_visibility.size(); w++ )
		visibility_texture_data_[ first_dynamic_walls_vertex_ / 2u + w ]= dynamic_walls_visibility[w] ? 255u : 0u;

	// Set arrow and framing visible.
	for( unsigned int i= 0u; i < g_arrow_lines; i++ )
		visibility_texture_data_[ arrow_vertices_offset_ / 2u + i ]= 255u;
	for( unsigned int i= 0u; i < g_framing_lines; i++ )
		visibility_texture_data_[ framing_vertices_offset_ / 2u + i ]= 255u;

	glBindTexture( GL_TEXTURE_1D, visibility_texture_id_ );
	glTexSubImage1D( GL_TEXTURE_1D, 0, 0, visibility_texture_data_.size(), GL_RED, GL_UNSIGNED_BYTE, visibility_texture_data_.data() );
}

} // namespace PanzerChasm
