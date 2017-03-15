#include <algorithm>
#include <cstring>

#include "rasterizer.hpp"

namespace PanzerChasm
{

Rasterizer::Rasterizer(
	const unsigned int viewport_size_x,
	const unsigned int viewport_size_y,
	const unsigned int row_size,
	uint32_t* const color_buffer )
	: viewport_size_x_( int(viewport_size_x) )
	, viewport_size_y_( int(viewport_size_y) )
	, row_size_( int(row_size) )
	, color_buffer_( color_buffer )
{
	depth_buffer_width_= ( viewport_size_x  + 1u ) & (~1u);
	depth_buffer_storage_.resize( depth_buffer_width_ * viewport_size_y );
	depth_buffer_= depth_buffer_storage_.data();
}

Rasterizer::~Rasterizer()
{}

void Rasterizer::ClearDepthBuffer()
{
	std::memset(
		depth_buffer_,
		0,
		depth_buffer_width_ * static_cast<unsigned int>(viewport_size_y_) * sizeof(unsigned short) );
}

void Rasterizer::SetTexture(
	const unsigned int size_x,
	const unsigned int size_y,
	const uint32_t* const data )
{
	texture_size_x_= size_x;
	texture_size_y_= size_y;
	texture_data_= data;
}

void Rasterizer::DrawAffineColoredTriangle( const RasterizerVertexSimple* const vertices, const uint32_t color )
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

	const fixed16_t long_edge_y_length= vertices[ upper_index ].y - vertices[ lower_index ].y;
	if( long_edge_y_length == 0 )
		return; // Triangle is unsignificant small. TODO - use other criteria.

	const fixed16_t middle_k=
			Fixed16Div( vertices[ middle_index ].y - vertices[ lower_index ].y, long_edge_y_length );

	// TODO - write "FixedMix" function to prevent overflow.
	const fixed16_t middle_x=
		Fixed16Mul( vertices[ upper_index ].x, middle_k ) +
		Fixed16Mul( vertices[ lower_index ].x, ( g_fixed16_one - middle_k ) );

	RasterizerVertexSimple middle_vertex;
	middle_vertex.x= middle_x;
	middle_vertex.y= vertices[ middle_index ].y;

	// TODO - optimize "triangle_part_vertices_simple_" assignment.
	if( middle_x >= vertices[ middle_index ].x )
	{
		/*
		   /\
		  /  \
		 /    \
		+_     \  <-
		   _    \
			 _   \
			   _  \
				 _ \
				   _\
		*/
		triangle_part_vertices_[0]= vertices[ lower_index ];
		triangle_part_vertices_[1]= vertices[ middle_index ];
		triangle_part_vertices_[2]= vertices[ lower_index ];
		triangle_part_vertices_[3]= middle_vertex;
		DrawAffineColoredTrianglePart( color );

		triangle_part_vertices_[0]= vertices[ middle_index ];
		triangle_part_vertices_[1]= vertices[ upper_index ];
		triangle_part_vertices_[2]= middle_vertex;
		triangle_part_vertices_[3]= vertices[ upper_index ];
		DrawAffineColoredTrianglePart( color );
	}
	else
	{
		/*
				/\
			   /  \
			  /    \
		->   /     _+
			/    _
		   /   _
		  /  _
		 / _
		/_
		*/
		triangle_part_vertices_[0]= vertices[ lower_index ];
		triangle_part_vertices_[1]= middle_vertex;
		triangle_part_vertices_[2]= vertices[ lower_index ];
		triangle_part_vertices_[3]= vertices[ middle_index ];
		DrawAffineColoredTrianglePart( color );

		triangle_part_vertices_[0]= middle_vertex;
		triangle_part_vertices_[1]= vertices[ upper_index ];
		triangle_part_vertices_[2]= vertices[ middle_index ];
		triangle_part_vertices_[3]= vertices[ upper_index ];
		DrawAffineColoredTrianglePart( color );
	}
}

void Rasterizer::DrawAffineTexturedTriangle( const RasterizerVertexTextured* vertices )
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

	const fixed_base_t lower_vertex_inv_z_scaled = Fixed16Div( c_inv_z_scaler << 16, vertices[ lower_index  ].z );
	const fixed_base_t middle_vertex_inv_z_scaled= Fixed16Div( c_inv_z_scaler << 16, vertices[ middle_index ].z );
	const fixed_base_t upper_vertex_inv_z_scaled = Fixed16Div( c_inv_z_scaler << 16, vertices[ upper_index  ].z );

	const fixed16_t long_edge_y_length= vertices[ upper_index ].y - vertices[ lower_index ].y;
	if( long_edge_y_length == 0 )
		return; // Triangle is unsignificant small. TODO - use other criteria.

	const fixed16_t middle_k=
			Fixed16Div( vertices[ middle_index ].y - vertices[ lower_index ].y, long_edge_y_length );

	// TODO - write "FixedMix" function to prevent overflow.
	const fixed16_t middle_x=
		Fixed16Mul( vertices[ upper_index ].x, middle_k ) +
		Fixed16Mul( vertices[ lower_index ].x, ( g_fixed16_one - middle_k ) );

	RasterizerVertexSimple middle_vertex;
	middle_vertex.x= middle_x;
	middle_vertex.y= vertices[ middle_index ].y;

	RasterizerTexCoord middle_vertex_tex_coord;
	middle_vertex_tex_coord.u=
		Fixed16Mul( vertices[ upper_index ].u, middle_k ) +
		Fixed16Mul( vertices[ lower_index ].u, ( g_fixed16_one - middle_k ) );
	middle_vertex_tex_coord.v=
		Fixed16Mul( vertices[ upper_index ].v, middle_k ) +
		Fixed16Mul( vertices[ lower_index ].v, ( g_fixed16_one - middle_k ) );

	const fixed_base_t middle_inv_z_scaled=
			Fixed16Mul( upper_vertex_inv_z_scaled, middle_k ) +
			Fixed16Mul( lower_vertex_inv_z_scaled, ( g_fixed16_one - middle_k ) );

	// TODO - optimize "triangle_part_vertices_simple_" assignment.
	if( middle_x >= vertices[ middle_index ].x )
	{
		/*
		   /\
		  /  \
		 /    \
		+_     \  <-
		   _    \
			 _   \
			   _  \
				 _ \
				   _\
		*/
		triangle_part_vertices_[0]= vertices[ lower_index ];
		triangle_part_vertices_[1]= vertices[ middle_index ];
		triangle_part_vertices_[2]= vertices[ lower_index ];
		triangle_part_vertices_[3]= middle_vertex;
		triangle_part_tex_coords_[0]= vertices[ lower_index ];
		triangle_part_tex_coords_[1]= vertices[ middle_index ];
		triangle_part_tex_coords_[2]= vertices[ lower_index ];
		triangle_part_tex_coords_[3]= middle_vertex_tex_coord;
		triangle_part_inv_z_scaled[0]= lower_vertex_inv_z_scaled;
		triangle_part_inv_z_scaled[1]= middle_vertex_inv_z_scaled;
		triangle_part_inv_z_scaled[2]= lower_vertex_inv_z_scaled;
		triangle_part_inv_z_scaled[3]= middle_inv_z_scaled;
		DrawAffineTexturedTrianglePart();

		triangle_part_vertices_[0]= vertices[ middle_index ];
		triangle_part_vertices_[1]= vertices[ upper_index ];
		triangle_part_vertices_[2]= middle_vertex;
		triangle_part_vertices_[3]= vertices[ upper_index ];
		triangle_part_tex_coords_[0]= vertices[ middle_index ];
		triangle_part_tex_coords_[1]= vertices[ upper_index ];
		triangle_part_tex_coords_[2]= middle_vertex_tex_coord;
		triangle_part_tex_coords_[3]= vertices[ upper_index ];
		triangle_part_inv_z_scaled[0]= middle_vertex_inv_z_scaled;
		triangle_part_inv_z_scaled[1]= upper_vertex_inv_z_scaled;
		triangle_part_inv_z_scaled[2]= middle_inv_z_scaled;
		triangle_part_inv_z_scaled[3]= upper_vertex_inv_z_scaled;
		DrawAffineTexturedTrianglePart();
	}
	else
	{
		/*
				/\
			   /  \
			  /    \
		->   /     _+
			/    _
		   /   _
		  /  _
		 / _
		/_
		*/
		triangle_part_vertices_[0]= vertices[ lower_index ];
		triangle_part_vertices_[1]= middle_vertex;
		triangle_part_vertices_[2]= vertices[ lower_index ];
		triangle_part_vertices_[3]= vertices[ middle_index ];
		triangle_part_tex_coords_[0]= vertices[ lower_index ];
		triangle_part_tex_coords_[1]= middle_vertex_tex_coord;
		triangle_part_tex_coords_[2]= vertices[ lower_index ];
		triangle_part_tex_coords_[3]= vertices[ middle_index ];
		triangle_part_inv_z_scaled[0]= lower_vertex_inv_z_scaled;
		triangle_part_inv_z_scaled[1]= middle_inv_z_scaled;
		triangle_part_inv_z_scaled[2]= lower_vertex_inv_z_scaled;
		triangle_part_inv_z_scaled[3]= middle_vertex_inv_z_scaled;
		DrawAffineTexturedTrianglePart();

		triangle_part_vertices_[0]= middle_vertex;
		triangle_part_vertices_[1]= vertices[ upper_index ];
		triangle_part_vertices_[2]= vertices[ middle_index ];
		triangle_part_vertices_[3]= vertices[ upper_index ];
		triangle_part_tex_coords_[0]= middle_vertex_tex_coord;
		triangle_part_tex_coords_[1]= vertices[ upper_index ];
		triangle_part_tex_coords_[2]= vertices[ middle_index ];
		triangle_part_tex_coords_[3]= vertices[ upper_index ];
		triangle_part_inv_z_scaled[0]= middle_inv_z_scaled;
		triangle_part_inv_z_scaled[1]= upper_vertex_inv_z_scaled;
		triangle_part_inv_z_scaled[2]= middle_vertex_inv_z_scaled;
		triangle_part_inv_z_scaled[3]= upper_vertex_inv_z_scaled;
		DrawAffineTexturedTrianglePart();
	}
}

void Rasterizer::DrawAffineColoredTrianglePart( const uint32_t color )
{
	const fixed16_t dy= triangle_part_vertices_[1].y - triangle_part_vertices_[0].y;
	if( dy <= 0 )
		return;

	const fixed16_t dx_left = triangle_part_vertices_[1].x - triangle_part_vertices_[0].x;
	const fixed16_t dx_right= triangle_part_vertices_[3].x - triangle_part_vertices_[2].x;
	const fixed16_t x_left_step = Fixed16Div( dx_left , dy );
	const fixed16_t x_right_step= Fixed16Div( dx_right, dy );

	PC_ASSERT( std::abs( Fixed16Mul( x_left_step , dy ) ) <= std::abs( dx_left  ) );
	PC_ASSERT( std::abs( Fixed16Mul( x_right_step, dy ) ) <= std::abs( dx_right ) );

	const int y_start= std::max( 0, Fixed16RoundToInt( triangle_part_vertices_[0].y ) );
	const int y_end  = std::min( viewport_size_y_, Fixed16RoundToInt( triangle_part_vertices_[1].y ) );

	const fixed16_t y_cut= ( y_start << 16 ) + g_fixed16_half - triangle_part_vertices_[0].y;
	fixed16_t x_left = triangle_part_vertices_[0].x + Fixed16Mul( y_cut, x_left_step  );
	fixed16_t x_right= triangle_part_vertices_[2].x + Fixed16Mul( y_cut, x_right_step );
	for(
		int y= y_start;
		y< y_end;
		y++, x_left+= x_left_step, x_right+= x_right_step )
	{
		const int x_start= std::max( 0, Fixed16RoundToInt( x_left ) );
		const int x_end= std::min( viewport_size_x_, Fixed16RoundToInt( x_right ) );

		uint32_t* dst= color_buffer_ + y * row_size_;
		for( int x= x_start; x < x_end; x++ )
		{
			dst[x]= color;
		}
	} // for y
}

void Rasterizer::DrawAffineTexturedTrianglePart()
{
	const fixed16_t dy= triangle_part_vertices_[1].y - triangle_part_vertices_[0].y;
	if( dy <= 0 )
		return;

	const fixed16_t dx_left = triangle_part_vertices_[1].x - triangle_part_vertices_[0].x;
	const fixed16_t dx_right= triangle_part_vertices_[3].x - triangle_part_vertices_[2].x;
	const fixed16_t x_left_step = Fixed16Div( dx_left , dy );
	const fixed16_t x_right_step= Fixed16Div( dx_right, dy );

	PC_ASSERT( std::abs( Fixed16Mul( x_left_step , dy ) ) <= std::abs( dx_left  ) );
	PC_ASSERT( std::abs( Fixed16Mul( x_right_step, dy ) ) <= std::abs( dx_right ) );

	fixed16_t d_tc_left[2], d_tc_right[2], tc_left_step[2], tc_right_step[2];
	d_tc_left [0]= triangle_part_tex_coords_[1].u - triangle_part_tex_coords_[0].u;
	d_tc_left [1]= triangle_part_tex_coords_[1].v - triangle_part_tex_coords_[0].v;
	d_tc_right[0]= triangle_part_tex_coords_[3].u - triangle_part_tex_coords_[2].u;
	d_tc_right[1]= triangle_part_tex_coords_[3].v - triangle_part_tex_coords_[2].v;
	tc_left_step [0]= Fixed16Div( d_tc_left [0], dy );
	tc_left_step [1]= Fixed16Div( d_tc_left [1], dy );
	tc_right_step[0]= Fixed16Div( d_tc_right[0], dy );
	tc_right_step[1]= Fixed16Div( d_tc_right[1], dy );

	const fixed_base_t d_inv_z_scaled_left =triangle_part_inv_z_scaled[1] - triangle_part_inv_z_scaled[0];
	const fixed_base_t d_inv_z_scaled_right=triangle_part_inv_z_scaled[3] - triangle_part_inv_z_scaled[2];
	const fixed_base_t inv_z_scaled_left_step = Fixed16Div( d_inv_z_scaled_left , dy );
	const fixed_base_t inv_z_scaled_right_step= Fixed16Div( d_inv_z_scaled_right, dy );

	const int y_start= std::max( 0, Fixed16RoundToInt( triangle_part_vertices_[0].y ) );
	const int y_end  = std::min( viewport_size_y_, Fixed16RoundToInt( triangle_part_vertices_[1].y ) );

	const fixed16_t y_cut= ( y_start << 16 ) + g_fixed16_half - triangle_part_vertices_[0].y;
	fixed16_t x_left = triangle_part_vertices_[0].x + Fixed16Mul( y_cut, x_left_step  );
	fixed16_t x_right= triangle_part_vertices_[2].x + Fixed16Mul( y_cut, x_right_step );

	fixed16_t tc_left[2], tc_right[2];
	tc_left [0]= triangle_part_tex_coords_[0].u + Fixed16Mul( y_cut, tc_left_step [0]  );
	tc_left [1]= triangle_part_tex_coords_[0].v + Fixed16Mul( y_cut, tc_left_step [1]  );
	tc_right[0]= triangle_part_tex_coords_[2].u + Fixed16Mul( y_cut, tc_right_step[0]  );
	tc_right[1]= triangle_part_tex_coords_[2].v + Fixed16Mul( y_cut, tc_right_step[1]  );

	fixed_base_t inv_z_scaled_left = triangle_part_inv_z_scaled[0] + Fixed16Mul( y_cut, inv_z_scaled_left_step  );
	fixed_base_t inv_z_scaled_right= triangle_part_inv_z_scaled[2] + Fixed16Mul( y_cut, inv_z_scaled_right_step );

	for(
		int y= y_start;
		y< y_end;
		y++,
		x_left+= x_left_step, x_right+= x_right_step,
		tc_left [0]+= tc_left_step [0], tc_left [1]+= tc_left_step [1],
		tc_right[0]+= tc_right_step[0], tc_right[1]+= tc_right_step[1],
		inv_z_scaled_left+= inv_z_scaled_left_step, inv_z_scaled_right+= inv_z_scaled_right_step )
	{
		const int x_start= std::max( 0, Fixed16RoundToInt( x_left ) );
		const int x_end= std::min( viewport_size_x_, Fixed16RoundToInt( x_right ) );

		fixed16_t line_tc_step[2];
		const fixed16_t line_dx= x_right - x_left;
		if( line_dx <= 0 )
			continue;
		line_tc_step[0]= Fixed16Div( tc_right[0] - tc_left[0], line_dx );
		line_tc_step[1]= Fixed16Div( tc_right[1] - tc_left[1], line_dx );

		const fixed_base_t line_inv_z_scaled_step= Fixed16Div( inv_z_scaled_right - inv_z_scaled_left, line_dx );

		const fixed16_t x_cut= ( x_start << 16 ) + g_fixed16_half - x_left;

		fixed16_t line_tc[2];
		line_tc[0]= tc_left[0] + Fixed16Mul( x_cut, line_tc_step[0] );
		line_tc[1]= tc_left[1] + Fixed16Mul( x_cut, line_tc_step[1] );

		fixed16_t line_inv_z_scaled= inv_z_scaled_left + Fixed16Mul( x_cut, line_inv_z_scaled_step );

		uint32_t* dst= color_buffer_ + y * row_size_;
		unsigned short* depth_dst= depth_buffer_ + y * depth_buffer_width_;

		for( int x= x_start; x < x_end; x++,
			line_tc[0]+= line_tc_step[0], line_tc[1]+= line_tc_step[1],
			 line_inv_z_scaled+= line_inv_z_scaled_step )
		{
			// TODO - check this.
			// "depth" must be 65536 when inv_z == ( 1 << c_max_inv_z_min_log2 )
			const unsigned short depth= line_inv_z_scaled >> ( c_inv_z_scaler_log2 + c_max_inv_z_min_log2 );

			if( depth > depth_dst[x] )
			{
				depth_dst[x]= depth;

				int u= line_tc[0] >> 16;
				int v= line_tc[1] >> 16;
				// TODO - remove range check from here.
				// Make range check earlier.
				if( u >= 0 && v >= 0 && u < texture_size_x_ && v < texture_size_y_ )
					dst[x]= texture_data_[ u + v * texture_size_x_ ];
				else
					dst[x]= 0x00FF00FFu;
			}
		}
	} // for y
}

} // namespace PanzerChasm
