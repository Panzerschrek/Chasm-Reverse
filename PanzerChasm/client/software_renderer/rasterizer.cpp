#include <algorithm>

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
}

Rasterizer::~Rasterizer()
{}

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
		triangle_part_vertices_simple_[0]= vertices[ lower_index ];
		triangle_part_vertices_simple_[1]= vertices[ middle_index ];
		triangle_part_vertices_simple_[2]= vertices[ lower_index ];
		triangle_part_vertices_simple_[3]= middle_vertex;
		DrawAffineColoredTrianglePart( color );

		triangle_part_vertices_simple_[0]= vertices[ middle_index ];
		triangle_part_vertices_simple_[1]= vertices[ upper_index ];
		triangle_part_vertices_simple_[2]= middle_vertex;
		triangle_part_vertices_simple_[3]= vertices[ upper_index ];
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
		triangle_part_vertices_simple_[0]= vertices[ lower_index ];
		triangle_part_vertices_simple_[1]= middle_vertex;
		triangle_part_vertices_simple_[2]= vertices[ lower_index ];
		triangle_part_vertices_simple_[3]= vertices[ middle_index ];
		DrawAffineColoredTrianglePart( color );

		triangle_part_vertices_simple_[0]= middle_vertex;
		triangle_part_vertices_simple_[1]= vertices[ upper_index ];
		triangle_part_vertices_simple_[2]= vertices[ middle_index ];
		triangle_part_vertices_simple_[3]= vertices[ upper_index ];
		DrawAffineColoredTrianglePart( color );
	}
}

void Rasterizer::DrawAffineColoredTrianglePart( const uint32_t color )
{
	const fixed16_t dy= triangle_part_vertices_simple_[1].y - triangle_part_vertices_simple_[0].y;
	const fixed16_t dx_left = triangle_part_vertices_simple_[1].x - triangle_part_vertices_simple_[0].x;
	const fixed16_t dx_right= triangle_part_vertices_simple_[3].x - triangle_part_vertices_simple_[2].x;
	const fixed16_t x_left_step = Fixed16Div( dx_left , dy );
	const fixed16_t x_right_step= Fixed16Div( dx_right, dy );

	const int y_start= std::max( 0, Fixed16RoundToInt( triangle_part_vertices_simple_[0].y ) );
	const int y_end  = std::min( viewport_size_y_, Fixed16RoundToInt( triangle_part_vertices_simple_[1].y ) );

	const fixed16_t y_cut= ( y_start << 16 ) + g_fixed16_half - triangle_part_vertices_simple_[0].y;
	fixed16_t x_left = triangle_part_vertices_simple_[0].x + Fixed16Mul( y_cut, x_left_step  );
	fixed16_t x_right= triangle_part_vertices_simple_[2].x + Fixed16Mul( y_cut, x_right_step );
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

} // namespace PanzerChasm
