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
	{ // Setup depth buffer and depth buffer hierarchy.
		unsigned int memory_for_depth_required= 0u;
		depth_buffer_width_= ( viewport_size_x + 1u ) & (~1u);
		const unsigned int depth_buffer_memory_size= depth_buffer_width_ * viewport_size_y;
		memory_for_depth_required+= depth_buffer_memory_size;

		for( unsigned int i= 0u; i < c_depth_buffer_hierarchy_levels; i++ )
		{
			const unsigned int hierarchy_cell_size= c_first_depth_hierarchy_level_size << i;
			depth_buffer_hierarchy_[i].width = ( depth_buffer_width_ + ( hierarchy_cell_size - 1u ) ) / hierarchy_cell_size;
			depth_buffer_hierarchy_[i].height= (    viewport_size_y_ + ( hierarchy_cell_size - 1u ) ) / hierarchy_cell_size;

			memory_for_depth_required+= depth_buffer_hierarchy_[i].width * depth_buffer_hierarchy_[i].height;
		}

		depth_buffer_storage_.resize( memory_for_depth_required );

		unsigned int offset= 0u;
		depth_buffer_= depth_buffer_storage_.data();
		offset+= depth_buffer_memory_size;

		for( unsigned int i= 0u; i < c_depth_buffer_hierarchy_levels; i++ )
		{
			depth_buffer_hierarchy_[i].data= depth_buffer_storage_.data() + offset;
			offset+= depth_buffer_hierarchy_[i].width * depth_buffer_hierarchy_[i].height;
		}
	}
	{ // Setup occlusion buffer
		occlusion_buffer_width_= ( ( viewport_size_x + 63 ) & (~63) ) / 8;
		occlusion_buffer_storage_.resize( occlusion_buffer_width_ * viewport_size_y_ );
		occlusion_buffer_= occlusion_buffer_storage_.data();
	}
}

Rasterizer::~Rasterizer()
{}

void Rasterizer::ClearDepthBuffer()
{
	std::memset(
		depth_buffer_,
		0,
		static_cast<unsigned int>( depth_buffer_width_ * viewport_size_y_) * sizeof(unsigned short) );

	std::memset(
		occlusion_buffer_,
		0,
		static_cast<unsigned int>( occlusion_buffer_width_ * viewport_size_y_ ) );
}

void Rasterizer::BuildDepthBufferHierarchy()
{
	const unsigned int first_level_size_truncated_x= static_cast<unsigned int>( viewport_size_x_ ) / c_first_depth_hierarchy_level_size;
	const unsigned int first_level_size_truncated_y= static_cast<unsigned int>( viewport_size_y_ ) / c_first_depth_hierarchy_level_size;
	const unsigned int first_level_x_left= static_cast<unsigned int>( viewport_size_x_ ) % c_first_depth_hierarchy_level_size;
	const unsigned int first_level_y_left= static_cast<unsigned int>( viewport_size_y_ ) % c_first_depth_hierarchy_level_size;

	for( unsigned int y= 0u; y < first_level_size_truncated_y; y++ )
	{
		const unsigned short* src[ c_first_depth_hierarchy_level_size ];
		for( unsigned int i= 0u; i < c_first_depth_hierarchy_level_size; i++ )
			src[i]= depth_buffer_ + ( y * c_first_depth_hierarchy_level_size + i ) * static_cast<unsigned int>(depth_buffer_width_);

		unsigned short* const dst= depth_buffer_hierarchy_[0].data + y * depth_buffer_hierarchy_[0].width;

		for( unsigned int x= 0u; x < first_level_size_truncated_x; x++ )
		{
			unsigned short min_depth= 0xFFFFu;
			for( unsigned int dy= 0u; dy < c_first_depth_hierarchy_level_size; dy++ )
			for( unsigned int dx= 0u; dx < c_first_depth_hierarchy_level_size; dx++ )
				min_depth= std::min( min_depth, src[dy][ x * c_first_depth_hierarchy_level_size + dx] );
			dst[x]= min_depth;
		}

		// Last partial column.
		if( first_level_x_left > 0u )
		{
			const unsigned int x= first_level_size_truncated_x;

			unsigned short min_depth= 0xFFFFu;
			for( unsigned int dy= 0u; dy < c_first_depth_hierarchy_level_size; dy++ )
			for( unsigned int dx= 0u; dx < first_level_x_left; dx++ )
				min_depth= std::min( min_depth, src[dy][ x * c_first_depth_hierarchy_level_size + dx ] );
			dst[x]= min_depth;
		}
	}

	// Last partial row.
	if( first_level_y_left > 0u )
	{
		const unsigned int y= first_level_size_truncated_y;
		PC_ASSERT( y == depth_buffer_hierarchy_[0].height - 1u );

		const unsigned short* src[ c_first_depth_hierarchy_level_size ];
		for( unsigned int i= 0u; i < first_level_y_left; i++ )
			src[i]= depth_buffer_ + ( y * c_first_depth_hierarchy_level_size + i ) * static_cast<unsigned int>(depth_buffer_width_);

		unsigned short* const dst= depth_buffer_hierarchy_[0].data + y * depth_buffer_hierarchy_[0].width;

		for( unsigned int x= 0u; x < first_level_size_truncated_x; x++ )
		{
			unsigned short min_depth= 0xFFFFu;
			for( unsigned int dy= 0u; dy < first_level_y_left; dy++ )
			for( unsigned int dx= 0u; dx < c_first_depth_hierarchy_level_size; dx++ )
				min_depth= std::min( min_depth, src[dy][ x * c_first_depth_hierarchy_level_size + dx ] );
			dst[x]= min_depth;
		}

		// Last partial column.
		if( first_level_x_left > 0u )
		{
			const unsigned int x= first_level_size_truncated_x;

			unsigned short min_depth= 0xFFFFu;
			for( unsigned int dy= 0u; dy < first_level_y_left; dy++ )
			for( unsigned int dx= 0u; dx < first_level_x_left; dx++ )
				min_depth= std::min( min_depth, src[dy][ x * c_first_depth_hierarchy_level_size + dx ] );
			dst[x]= min_depth;
		}
	}

	// Fill hierarchy levels
	for( unsigned int i= 1u; i < c_depth_buffer_hierarchy_levels; i++ )
	{
		const unsigned int size_truncated_x= depth_buffer_hierarchy_[i-1u].width  / 2u;
		const unsigned int size_truncated_y= depth_buffer_hierarchy_[i-1u].height / 2u;
		const unsigned int x_left= depth_buffer_hierarchy_[i-1u].width  % 2u;
		const unsigned int y_left= depth_buffer_hierarchy_[i-1u].height % 2u;

		for( unsigned int y= 0u; y < size_truncated_y; y++ )
		{
			const unsigned short* const src[2]=
			{
				depth_buffer_hierarchy_[i-1u].data + (y*2u   ) * depth_buffer_hierarchy_[i-1u].width,
				depth_buffer_hierarchy_[i-1u].data + (y*2u+1u) * depth_buffer_hierarchy_[i-1u].width,
			};
			unsigned short* const dst= depth_buffer_hierarchy_[i].data + y * depth_buffer_hierarchy_[i].width;

			for( unsigned int x= 0u; x < size_truncated_x; x++ )
				dst[x]=
					std::min(
						std::min( src[0][x*2u], src[0][x*2u+1u] ),
						std::min( src[1][x*2u], src[1][x*2u+1u] ) );

			// Last partial column.
			if( x_left > 0u )
			{
				PC_ASSERT( x_left == 1u );
				dst[ size_truncated_x ]=
					std::min( src[0][ size_truncated_x * 2u ], src[1][ size_truncated_x * 2u ] );
			}
		}

		// Last partial row.
		if( y_left > 0u )
		{
			PC_ASSERT( y_left == 1u );
			const unsigned int y= size_truncated_y;

			const unsigned short* const src= depth_buffer_hierarchy_[i-1u].data + (y*2u) * depth_buffer_hierarchy_[i-1u].width;
			unsigned short* const dst= depth_buffer_hierarchy_[i].data + y * depth_buffer_hierarchy_[i].width;

			for( unsigned int x= 0u; x < size_truncated_x; x++ )
				dst[x]= std::min( src[x*2u], src[x*2u+1u] );

			// Last partial column.
			if( x_left > 0u )
			{
				PC_ASSERT( x_left == 1u );
				dst[ size_truncated_x ]= src[ size_truncated_x * 2u ];
			}
		}
	} // for hierarchy levels
}

bool Rasterizer::IsDepthOccluded(
	fixed16_t x_min, fixed16_t y_min, fixed16_t x_max, fixed16_t y_max,
	fixed16_t z_min, fixed16_t z_max ) const
{
	PC_UNUSED( z_max );
	const unsigned short depth= Fixed16Div( g_fixed16_one >> c_max_inv_z_min_log2, z_min );

	PC_ASSERT( x_min <= x_max );
	PC_ASSERT( y_min <= y_max );
	// Ceil delta to nearest integer.
	const int dx= ( ( x_max - x_min ) + g_fixed16_one_minus_eps ) >> 16;
	const int dy= ( ( y_max - y_min ) + g_fixed16_one_minus_eps ) >> 16;
	const int max_delta= std::max( dx, dy );

	// TODO - calibrate hierarchy level selection.
	// After level selection we must fetch not so much depth values from hierarchy.
	int hierarchy_level= 0u;
	do
	{
		hierarchy_level++;
	} while( max_delta / 4 > ( int(c_first_depth_hierarchy_level_size) << hierarchy_level ) );

	if( hierarchy_level >= int(c_depth_buffer_hierarchy_levels) )
		hierarchy_level= int(c_depth_buffer_hierarchy_levels) - 1;

	const auto& depth_hierarchy= depth_buffer_hierarchy_[ hierarchy_level ];
	const int hierarchy_x_min= std::max( ( x_min >> ( 16 + hierarchy_level ) ) / int(c_first_depth_hierarchy_level_size), 0 );
	const int hierarchy_y_min= std::max( ( y_min >> ( 16 + hierarchy_level ) ) / int(c_first_depth_hierarchy_level_size), 0 );
	const int hierarchy_x_max= std::min( ( x_max >> ( 16 + hierarchy_level ) ) / int(c_first_depth_hierarchy_level_size), int(depth_hierarchy.width ) - 1 );
	const int hierarchy_y_max= std::min( ( y_max >> ( 16 + hierarchy_level ) ) / int(c_first_depth_hierarchy_level_size), int(depth_hierarchy.height) - 1 );

	for( int y= hierarchy_y_min; y <= hierarchy_y_max; y++ )
	for( int x= hierarchy_x_min; x <= hierarchy_x_max; x++ )
	{
		if( depth_hierarchy.data[ x + y * int(depth_hierarchy.width) ] < depth )
			return false;
	}

	// Debug output - draw screen space bounding box if occluded.
	/*
	{
		const int scale= int(c_first_depth_hierarchy_level_size) << hierarchy_level;
		const int x_start= std::max( hierarchy_x_min * scale, 0 );
		const int x_end  = std::min( (hierarchy_x_max+1) * scale, viewport_size_x_ );
		const int y_start= std::max( hierarchy_y_min * scale, 0 );
		const int y_end  = std::min( (hierarchy_y_max+1) * scale , viewport_size_y_ );
		for( int y= y_start; y < y_end; y++ )
		for( int x= x_start; x < x_end; x++ )
			color_buffer_[ x + y * row_size_ ]= 0xFFFF0000u;
	}
	{
		const int x_start= std::max( x_min >> 16, 0 );
		const int x_end  = std::min( x_max >> 16, viewport_size_x_ );
		const int y_start= std::max( y_min >> 16, 0 );
		const int y_end  = std::min( y_max >> 16, viewport_size_y_ );
		for( int y= y_start; y < y_end; y++ )
		for( int x= x_start; x < x_end; x++ )
			color_buffer_[ x + y * row_size_ ]= 0xFF00FF00u;
	}
	*/

	return true;
}

void Rasterizer::DebugDrawDepthHierarchy( unsigned int tick_count )
{
	const auto depth_to_color=
	[]( const unsigned short depth ) -> uint32_t
	{
		unsigned int d= depth;
		d*= 4u;
		if( d > 65535u )d= 65535u;
		d>>= 8u;
		return d | (d<<8u) | (d<<16u) | (d<<24u);
	};

	unsigned int level= (1u + tick_count) % ( c_depth_buffer_hierarchy_levels + 2u );

	if( level == 0u )
		return;
	else if( level == 1u )
	{
		for( int y= 0u; y < viewport_size_y_; y++ )
		for( int x= 0u; x < viewport_size_x_; x++ )
			color_buffer_[ x + y * row_size_ ]=
				depth_to_color( depth_buffer_[ x + y * depth_buffer_width_ ] );
	}
	else
	{
		level-= 2u;

		const auto& depth_hierarchy= depth_buffer_hierarchy_[ level ];
		const int div= c_first_depth_hierarchy_level_size << int(level);

		for( int y= 0u; y < viewport_size_y_; y++ )
		for( int x= 0u; x < viewport_size_x_; x++ )
			color_buffer_[ x + y * row_size_ ]=
				depth_to_color( depth_hierarchy.data[ x/div + y/div * int(depth_hierarchy.width) ] );
	}
}

void Rasterizer::DebugDrawOcclusionBuffer( unsigned int tick_count )
{
	const bool draw= (tick_count&1u) != 0u;
	if( !draw ) return;

	for( int y= 0u; y < viewport_size_y_; y++ )
	for( int x= 0u; x < viewport_size_x_; x++ )
		color_buffer_[ x + y * row_size_ ]=
			( occlusion_buffer_[ (x>>3) + y * occlusion_buffer_width_ ] & (1<<(x&7)) ) == 0u
				? 0x00000000u
				: 0xFFFFFFFFu;
}

void Rasterizer::SetTexture(
	const unsigned int size_x,
	const unsigned int size_y,
	const uint32_t* const data )
{
	texture_size_x_= size_x;
	texture_size_y_= size_y;
	max_valid_tc_u_ = ( texture_size_x_ << 16 ) - 1;
	max_valid_tc_v_ = ( texture_size_y_ << 16 ) - 1;
	texture_data_= data;
}

void Rasterizer::DrawAffineColoredTriangle( const RasterizerVertex* const vertices, const uint32_t color )
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

	RasterizerVertexCoord middle_vertex;
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

} // namespace PanzerChasm
