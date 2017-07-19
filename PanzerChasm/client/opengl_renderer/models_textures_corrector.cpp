#include "models_textures_corrector.hpp"

namespace PanzerChasm
{

static const unsigned char g_empty_field_value= 0u;
static const unsigned char g_nonempty_field_value= 1u;

ModelsTexturesCorrector::ModelsTexturesCorrector()
{
}

ModelsTexturesCorrector::~ModelsTexturesCorrector()
{
}

void ModelsTexturesCorrector::CorrectModelTexture(
	const Model& model,
	unsigned char* const texture_data_rgba )
{
	field_.clear();
	field_.resize( model.texture_size[0] * model.texture_size[1], g_empty_field_value );

	texture_size_[0]= model.texture_size[0];
	texture_size_[1]= model.texture_size[1];
	texture_data_rgba_= texture_data_rgba;

	const m_Vec2 tc_scale( float(model.texture_size[0]), float(model.texture_size[1]) );

	const auto draw_triangles=
	[&]( const std::vector<unsigned short>& indeces )
	{
		for( unsigned int i= 0u; i < indeces.size(); i+= 3 )
		{
			const bool alpha_test = model.vertices[ indeces[i] ].alpha_test_mask != 0u;

			m_Vec2 src_vertices[3];
			m_Vec2 triangle_center( 0.0f, 0.0f );
			for( unsigned int j= 0u; j < 3u; j++ )
			{
				src_vertices[j]= m_Vec2( model.vertices[ indeces[i + j] ].tex_coord );
				src_vertices[j]*= tc_scale;
				triangle_center+= src_vertices[j];
			}
			triangle_center*= 1.0f / 3.0f;

			m_Vec2 vertices_shifted[6u];
			for( unsigned int j= 0u; j < 3u; j++ )
			{
				const m_Vec2 edge_vec= src_vertices[(j+1u)%3u] - src_vertices[j];
				m_Vec2 shift_vec(-edge_vec.y, edge_vec.x);
				shift_vec.Normalize();

				// Shift must be from triangle center.
				if( shift_vec * ( src_vertices[j] - triangle_center ) < 0.0f )
					shift_vec= -shift_vec;

				// shift edge, using distance to nearest pixel.
				shift_vec*= 0.5f / std::max( std::abs(shift_vec.x), std::abs(shift_vec.y) );
				// Make just a bit less shift.
				shift_vec*= 0.8f;

				vertices_shifted[j*2u+0u] = src_vertices[j] + shift_vec;
				vertices_shifted[j*2u+1u] = src_vertices[(j+1u)%3u] + shift_vec;
			}

			m_Vec2 triangle_vertices[3];
			triangle_vertices[0]= vertices_shifted[5];
			triangle_vertices[1]= vertices_shifted[0];
			triangle_vertices[2]= vertices_shifted[1];
			DrawTriangle( triangle_vertices, alpha_test );
			triangle_vertices[0]= vertices_shifted[1];
			triangle_vertices[1]= vertices_shifted[2];
			triangle_vertices[2]= vertices_shifted[3];
			DrawTriangle( triangle_vertices, alpha_test );
			triangle_vertices[0]= vertices_shifted[3];
			triangle_vertices[1]= vertices_shifted[4];
			triangle_vertices[2]= vertices_shifted[5];
			DrawTriangle( triangle_vertices, alpha_test );
			triangle_vertices[0]= vertices_shifted[5];
			triangle_vertices[1]= vertices_shifted[1];
			triangle_vertices[2]= vertices_shifted[3];
			DrawTriangle( triangle_vertices, alpha_test );
		}
	};

	draw_triangles( model.regular_triangles_indeces );
	draw_triangles( model.transparent_triangles_indeces );

	for( int y = 0; y < texture_size_[1]; y++ )
	for( int x = 0; x < texture_size_[0]; x++ )
	{
		if( field_[ x + y * texture_size_[0] ] != g_empty_field_value )
			continue;

		unsigned int nonempty_neighbors= 0u;
		unsigned int avg_color[3]= { 0u, 0u, 0u };
		if( x > 0 )
		{
			const int address= x - 1 + y * texture_size_[0];
			if( field_[address] != g_empty_field_value)
			{
				nonempty_neighbors++;
				for( unsigned int j= 0u; j < 3u; j++ )
					avg_color[j]+= texture_data_rgba[ address * 4 + int(j) ];
			}
		}
		if( x + 1 < texture_size_[0] )
		{
			const int address= x + 1 + y * texture_size_[0];
			if( field_[address] != g_empty_field_value )
			{
				nonempty_neighbors++;
				for( unsigned int j= 0u; j < 3u; j++ )
					avg_color[j]+= texture_data_rgba[ address * 4 + int(j) ];
			}
		}
		if( y > 0 )
		{
			const int address= x + (y - 1) * texture_size_[0];
			if( field_[address] != g_empty_field_value )
			{
				nonempty_neighbors++;
				for( unsigned int j= 0u; j < 3u; j++ )
					avg_color[j]+= texture_data_rgba[ address * 4 + int(j) ];
			}
		}
		if( y + 1 < texture_size_[1] )
		{
			const int address= x + (y + 1) * texture_size_[0];
			if( field_[address] != g_empty_field_value )
			{
				nonempty_neighbors++;
				for( unsigned int j= 0u; j < 3u; j++ )
					avg_color[j]+= texture_data_rgba[ address * 4 + int(j) ];
			}
		}

		if( nonempty_neighbors > 0u )
		{
			unsigned char* const dst_texel= texture_data_rgba + ( x + y * texture_size_[0] ) * 4;
			for( unsigned int j= 0u; j < 3; j++ )
				dst_texel[j]= avg_color[j] / nonempty_neighbors;
			//dst_texel[0]= 255u;
		}
	}
}

void ModelsTexturesCorrector::DrawTriangle( const m_Vec2* const vertices, const bool alpha_test )
{
	// Sort triangle vertices.
	unsigned int upper_index;
	unsigned int middle_index;
	unsigned int lower_index;

	if( vertices[0].y >= vertices[1].y && vertices[0].y >= vertices[2].y )
	{
		upper_index= 0;
		lower_index= vertices[1].y < vertices[2].y ? 1u : 2u;
	}
	else if( vertices[1].y >= vertices[0].y && vertices[1].y >= vertices[2].y )
	{
		upper_index= 1;
		lower_index= vertices[0].y < vertices[2].y ? 0u : 2u;
	}
	else//if( vertices[2].y >= vertices[0].y && vertices[2].y >= vertices[1].y )
	{
		upper_index= 2;
		lower_index= vertices[0].y < vertices[1].y ? 0u : 1u;
	}
	middle_index= 0u + 1u + 2u - upper_index - lower_index;

	const float long_edge_y_length= vertices[ upper_index ].y - vertices[ lower_index ].y;
	if( long_edge_y_length == 0.0f )
		return;

	const float middle_k= ( vertices[ middle_index ].y - vertices[ lower_index ].y ) / long_edge_y_length;
	const float middle_x= vertices[ upper_index ].x * middle_k + vertices[ lower_index ].x * ( 1.0f - middle_k );
	const float middle_lower_dy= vertices[ middle_index ].y - vertices[ lower_index ].y;
	const float upper_middle_dy= vertices[ upper_index ].y - vertices[ middle_index ].y;
	if( middle_x >= vertices[ middle_index ].x )
	{
		triangle_part_x_step_right_= ( vertices[ upper_index ].x - vertices[ lower_index ].x ) / long_edge_y_length;
		if( middle_lower_dy > 0.0f )
		{
			triangle_part_x_step_left_= ( vertices[ middle_index ].x - vertices[ lower_index ].x ) / middle_lower_dy;
			triangle_part_vertices_[0]= vertices[ lower_index ];
			triangle_part_vertices_[1]= vertices[ middle_index ];
			triangle_part_vertices_[2]= vertices[ lower_index ];
			triangle_part_vertices_[3]= vertices[ upper_index ];
			DrawTrianglePart(alpha_test);
		}
		if( upper_middle_dy > 0.0f )
		{
			triangle_part_x_step_left_= ( vertices[ upper_index ].x - vertices[ middle_index ].x ) / upper_middle_dy ;
			triangle_part_vertices_[0]= vertices[ middle_index ];
			triangle_part_vertices_[1]= vertices[ upper_index ];
			triangle_part_vertices_[2]= vertices[ lower_index ];
			triangle_part_vertices_[3]= vertices[ upper_index ];
			DrawTrianglePart(alpha_test);
		}
	}
	else
	{
		triangle_part_x_step_left_= ( vertices[ upper_index ].x - vertices[ lower_index ].x ) / long_edge_y_length;
		if( middle_lower_dy > 0.0f )
		{
			triangle_part_x_step_right_= ( vertices[ middle_index ].x - vertices[ lower_index ].x ) / middle_lower_dy;
			triangle_part_vertices_[0]= vertices[ lower_index ];
			triangle_part_vertices_[1]= vertices[ upper_index ];
			triangle_part_vertices_[2]= vertices[ lower_index ];
			triangle_part_vertices_[3]= vertices[ middle_index ];
			DrawTrianglePart(alpha_test);
		}
		if( upper_middle_dy > 0.0f )
		{
			triangle_part_x_step_right_= ( vertices[ upper_index ].x - vertices[ middle_index ].x ) /  upper_middle_dy;
			triangle_part_vertices_[0]= vertices[ lower_index ];
			triangle_part_vertices_[1]= vertices[ upper_index ];
			triangle_part_vertices_[2]= vertices[ middle_index ];
			triangle_part_vertices_[3]= vertices[ upper_index ];
			DrawTrianglePart(alpha_test);
		}
	}
}

void  ModelsTexturesCorrector::DrawTrianglePart( const bool alpha_test )
{
	const float y_start_f= std::max( triangle_part_vertices_[0].y, triangle_part_vertices_[2].y );
	const float y_end_f  = std::min( triangle_part_vertices_[1].y, triangle_part_vertices_[3].y );
	const int y_start= std::max( 0, static_cast<int>( std::round( y_start_f ) ) );
	const int y_end  = std::min( static_cast<int>(texture_size_[1]), static_cast<int>( std::round( y_end_f ) ) );

	const float y_cut_left = static_cast<float>( y_start ) + 0.5f - triangle_part_vertices_[0].y;
	const float y_cut_right= static_cast<float>( y_start ) + 0.5f - triangle_part_vertices_[2].y;
	float x_left = triangle_part_vertices_[0].x + y_cut_left  * triangle_part_x_step_left_ ;
	float x_right= triangle_part_vertices_[2].x + y_cut_right * triangle_part_x_step_right_;
	for(
		int y= y_start;
		y< y_end;
		y++,
		x_left += triangle_part_x_step_left_ ,
		x_right+= triangle_part_x_step_right_ )
	{
		const int x_start= std::max( 0, static_cast<int>( std::round( x_left ) ) );
		const int x_end= std::min( static_cast<int>(texture_size_[0]), static_cast<int>( std::round( x_right ) ) );

		unsigned char* const dst= field_.data() + ( y * texture_size_[0] );
		if( alpha_test )
		{
			const unsigned char* const src= texture_data_rgba_ + ( y * texture_size_[0] ) * 4;
			for( int x= x_start; x < x_end; x++ )
			{
				if( src[ x * 4 + 3 ] > 128 )
					dst[x]= g_nonempty_field_value;
			}
		}
		else
		{
			for( int x= x_start; x < x_end; x++ )
				dst[x]= g_nonempty_field_value;
		}

	}
}

} // namespace PanzerChasm
