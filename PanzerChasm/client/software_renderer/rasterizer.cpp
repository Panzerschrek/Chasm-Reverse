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
	 // Setup occlusion buffer
	{
		const unsigned int top_hierarchy_level_cell_size= 16u << ( (c_occlusion_hierarchy_levels-1u) * 2u );
		const unsigned int top_hierarchy_level_cell_size_minus_one= top_hierarchy_level_cell_size - 1u;
		const unsigned int viewport_size_x_ceil= ( viewport_size_x + top_hierarchy_level_cell_size_minus_one ) / top_hierarchy_level_cell_size * top_hierarchy_level_cell_size;
		const unsigned int viewport_size_y_ceil= ( viewport_size_y + top_hierarchy_level_cell_size_minus_one ) / top_hierarchy_level_cell_size * top_hierarchy_level_cell_size;

		// Main buffer
		occlusion_buffer_width_ = viewport_size_x_ceil / 8u;
		occlusion_buffer_height_= viewport_size_y_ceil;
		occlusion_buffer_storage_.resize( occlusion_buffer_width_ * occlusion_buffer_height_ );
		occlusion_buffer_= occlusion_buffer_storage_.data();

		// Hierarchy
		unsigned int hexopixels_requested= 0u;
		for( unsigned int i= 0u; i < c_occlusion_hierarchy_levels; i++ )
		{
			auto& level= occlusion_hierarchy_levels_[i];
			const unsigned int heirarchy_cell_size= 16u << ( i * 2u );
			level.size[0]= ( viewport_size_x_ceil + ( heirarchy_cell_size - 1u ) ) / heirarchy_cell_size;
			level.size[1]= ( viewport_size_y_ceil + ( heirarchy_cell_size - 1u ) ) / heirarchy_cell_size;
			hexopixels_requested+= level.size[0] * level.size[1];
		}

		occlusion_heirarchy_storage_.resize( hexopixels_requested );
		unsigned int offset= 0u;
		for( unsigned int i= 0u; i < c_occlusion_hierarchy_levels; i++ )
		{
			auto& level= occlusion_hierarchy_levels_[i];
			level.data= occlusion_heirarchy_storage_.data() + offset;
			offset+= level.size[0] * level.size[1];
		}
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
}

void Rasterizer::ClearOcclusionBuffer()
{
	// Set all occlusion buffer to zero.
	std::memset(
		occlusion_buffer_,
		0,
		occlusion_buffer_storage_.size() );

	// Mark cells of occlusion buffer outside screen as "white".
	for( int y= 0; y < viewport_size_y_; y++ )
	{
		uint8_t* const dst= occlusion_buffer_ + y * occlusion_buffer_width_;
		const int x_ceil= ( viewport_size_x_ + 7 ) & (~7);

		for( int x= viewport_size_x_; x < x_ceil; x++ )
			dst[ x >> 3 ]|= 1 << (x&7);

		std::memset( dst + (x_ceil>>3), 0xFF, occlusion_buffer_width_ - (x_ceil>>3) );
	}
	for( int y= viewport_size_y_; y < occlusion_buffer_height_; y++ )
	{
		uint8_t* const dst= occlusion_buffer_ + y * occlusion_buffer_width_;
		std::memset( dst, 0xFF, occlusion_buffer_width_ );
	}

	// Set all occlusion hierarchy data to zero.
	std::memset( occlusion_heirarchy_storage_.data(), 0, occlusion_heirarchy_storage_.size() * sizeof(unsigned short) );

	// Mark as "white" hierarchy cells bits for subcells, outside screen.
	for( unsigned int i= 0u; i < c_occlusion_hierarchy_levels; i++ )
	{
		const auto& level= occlusion_hierarchy_levels_[i];
		const unsigned int cell_size_log2= 4u + (i*2u);
		const unsigned int cell_size= 1u << cell_size_log2;
		const unsigned int cell_size_minus_one= cell_size - 1u;

		unsigned int full_white_start_cell_x= ( static_cast<unsigned int>(viewport_size_x_) + cell_size_minus_one ) >> cell_size_log2;
		unsigned int full_white_start_cell_y= ( static_cast<unsigned int>(viewport_size_y_) + cell_size_minus_one ) >> cell_size_log2;
		const unsigned int full_black_end_cell_x= static_cast<unsigned int>(viewport_size_x_) >> cell_size_log2;
		const unsigned int full_black_end_cell_y= static_cast<unsigned int>(viewport_size_y_) >> cell_size_log2;

		for( unsigned int y= 0u; y < full_black_end_cell_y; y++ )
		{
			// Set partial cells bits to "one", if subcell is outside screen.
			for( unsigned int x= full_black_end_cell_x; x < full_white_start_cell_x; x++ )
			{
				unsigned short& cell_value= level.data[ x + y * level.size[0] ];
				for( unsigned int dx= 0u; dx < 4u; dx++ )
				for( unsigned int dy= 0u; dy < 4u; dy++ )
				{
					const unsigned int global_x= ( x << cell_size_log2 ) + ( dx << ( cell_size_log2 - 2u ) );
					if( global_x >= static_cast<unsigned int>(viewport_size_x_) )
						cell_value|= 1u << ( dx + dy * 4u );
				}
			}

			// Set full wite cells.
			for( unsigned int x= full_white_start_cell_x; x < level.size[0]; x++ )
				level.data[ x + y * level.size[0] ]= 0xFFFFu;
		}

		// Update partial Y cells.
		for( unsigned int y= full_black_end_cell_y; y < full_white_start_cell_y; y++ )
		for( unsigned int x= 0; x < level.size[0]; x++ )
		{
			unsigned short& cell_value= level.data[ x + y * level.size[0] ];
			for( unsigned int dy= 0u; dy < 4u; dy++ )
			for( unsigned int dx= 0u; dx < 4u; dx++ )
			{
				const unsigned int global_x= ( x << cell_size_log2 ) + ( dx << ( cell_size_log2 - 2u ) );
				const unsigned int global_y= ( y << cell_size_log2 ) + ( dy << ( cell_size_log2 - 2u ) );
				if( global_x >= static_cast<unsigned int>(viewport_size_x_) ||
					global_y >= static_cast<unsigned int>(viewport_size_y_) )
					cell_value|= 1u << ( dx + dy * 4u );
			}
		}

		// Set full white Y cells.
		for( unsigned int y= full_white_start_cell_y; y < level.size[1]; y++ )
			std::memset( level.data + y * level.size[0], 0xFF, level.size[0] * sizeof(unsigned short) );
	}
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
			const unsigned int src_x= x * c_first_depth_hierarchy_level_size;
			/*
			unsigned short min_depth= 0xFFFFu;
			for( unsigned int dy= 0u; dy < c_first_depth_hierarchy_level_size; dy++ )
			for( unsigned int dx= 0u; dx < c_first_depth_hierarchy_level_size; dx++ )
				min_depth= std::min( min_depth, src[dy][ src_x + dx ] );
			dst[x]= min_depth;
			*/

			// Manually unrolled min_depth calculation loop.
			// It contains less std::min calls (15 vs 16) and it has no data-dependent instructions sequentions.
			static_assert( c_first_depth_hierarchy_level_size == 4u, "You must chaqnge this, if you change c_first_depth_hierarchy_level_size." );
			const unsigned short d00= src[0][ src_x + 0 ];
			const unsigned short d01= src[0][ src_x + 1 ];
			const unsigned short d02= src[0][ src_x + 2 ];
			const unsigned short d03= src[0][ src_x + 3 ];
			const unsigned short d10= src[1][ src_x + 0 ];
			const unsigned short d11= src[1][ src_x + 1 ];
			const unsigned short d12= src[1][ src_x + 2 ];
			const unsigned short d13= src[1][ src_x + 3 ];
			const unsigned short d20= src[2][ src_x + 0 ];
			const unsigned short d21= src[2][ src_x + 1 ];
			const unsigned short d22= src[2][ src_x + 2 ];
			const unsigned short d23= src[2][ src_x + 3 ];
			const unsigned short d30= src[3][ src_x + 0 ];
			const unsigned short d31= src[3][ src_x + 1 ];
			const unsigned short d32= src[3][ src_x + 2 ];
			const unsigned short d33= src[3][ src_x + 3 ];
			const unsigned short min00_01= std::min( d00, d01 );
			const unsigned short min02_03= std::min( d02, d03 );
			const unsigned short min10_11= std::min( d10, d11 );
			const unsigned short min12_13= std::min( d12, d13 );
			const unsigned short min20_21= std::min( d20, d21 );
			const unsigned short min22_23= std::min( d22, d23 );
			const unsigned short min30_31= std::min( d30, d31 );
			const unsigned short min32_33= std::min( d32, d33 );
			const unsigned short min0= std::min( min00_01, min02_03 );
			const unsigned short min1= std::min( min10_11, min12_13 );
			const unsigned short min2= std::min( min20_21, min22_23 );
			const unsigned short min3= std::min( min30_31, min32_33 );
			const unsigned short min_l= std::min( min0, min1 );
			const unsigned short min_h= std::min( min2, min3 );
			dst[x]= std::min( min_l, min_h );
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
	PC_ASSERT( z_min > ( g_fixed16_one >> c_max_inv_z_min_log2 ) );
	PC_ASSERT( z_max > ( g_fixed16_one >> c_max_inv_z_min_log2 ) );
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

template<unsigned int level>
unsigned int Rasterizer::UpdateOcclusionHierarchyCell_r( const unsigned int cell_x, const unsigned int cell_y )
{
	static_assert( level < c_occlusion_hierarchy_levels, "Invalid level" );
	auto& hierarchy_level= occlusion_hierarchy_levels_[level];
	PC_ASSERT( cell_x < hierarchy_level.size[0] );
	PC_ASSERT( cell_y < hierarchy_level.size[1] );
	unsigned short& cell_value= hierarchy_level.data[ cell_x + cell_y * hierarchy_level.size[0] ];

	if( level == 0u )
	{
		for( unsigned int subcell_y= 0u; subcell_y < 4u; subcell_y++ )
		for( unsigned int subcell_x= 0u; subcell_x < 4u; subcell_x++ )
		{
			const unsigned int bit_number= subcell_x + ( subcell_y << 2u );
			const unsigned int bit_mask= 1u << bit_number;
			if( ( cell_value & bit_mask ) != 0u )
				continue;

			const unsigned int global_x_start= cell_x * 16u + subcell_x * 4u;
			const unsigned int global_y_start= cell_y * 16u + subcell_y * 4u;

			/* // Older slow, but more readable version.
			unsigned int bit4x4= 1u;
			for( unsigned int bit_y= 0u; bit_y < 4u; bit_y++ )
			for( unsigned int bit_x= 0u; bit_x < 4u; bit_x++ )
			{
				const unsigned int global_x= global_x_start + bit_x;
				const unsigned int global_y= global_y_start + bit_y;
				PC_ASSERT( global_x < static_cast<unsigned int>( occlusion_buffer_width_ << 3 ) );
				PC_ASSERT( global_y < static_cast<unsigned int>( occlusion_buffer_height_ ) );

				const unsigned int bit= global_x & 7u;
				// TODO - maybe (global_x >> 3u) constant in this loop ?
				bit4x4&= occlusion_buffer_[ (global_x >> 3u) + global_y * occlusion_buffer_width_ ] >> bit;
			}
			cell_value|= (bit4x4&1u) << bit_number;
			*/

			unsigned int bits4x4= 255u; // bits [0-3] or bits [4-7] of "bits4x4" used, other bits is unsignificant.
			const unsigned int occlusion_byte_x= global_x_start >> 3u;
			const unsigned int occlusion_byte_shift= global_x_start & 7u;
			for( unsigned int bit_y= 0u; bit_y < 4u; bit_y++ )
			{
				// Fetch 4 x-bits of occlusion byte each iteration.
				const unsigned int global_y= global_y_start + bit_y;
				bits4x4&= occlusion_buffer_[ occlusion_byte_x + global_y * static_cast<unsigned int>(occlusion_buffer_width_) ];
			}
			cell_value|= ( ( (bits4x4 >> occlusion_byte_shift) & 15u) == 15u ? 1u : 0u ) << bit_number;
		}
	}
	else
	{
		for( unsigned int subcell_y= 0u; subcell_y < 4u; subcell_y++ )
		for( unsigned int subcell_x= 0u; subcell_x < 4u; subcell_x++ )
		{
			const unsigned int bit_number= subcell_x + ( subcell_y << 2u );
			const unsigned int bit_mask= 1u << bit_number;
			if( ( cell_value & bit_mask ) != 0u )
				continue;

				cell_value|=
					UpdateOcclusionHierarchyCell_r< level == 0u ? 0u : level - 1u >(
						cell_x * 4u + subcell_x,
						cell_y * 4u + subcell_y ) << bit_number;
		}
	}

	return cell_value == 0xFFFFu ? 1u : 0u;
}

template<unsigned int level>
void Rasterizer::SetToOneOcclusionHierarchyCell_r( const unsigned int cell_x, const unsigned int cell_y )
{
	static_assert( level < c_occlusion_hierarchy_levels, "Invalid level" );
	auto& hierarchy_level= occlusion_hierarchy_levels_[level];
	PC_ASSERT( cell_x < hierarchy_level.size[0] );
	PC_ASSERT( cell_y < hierarchy_level.size[1] );
	unsigned short& cell_value= hierarchy_level.data[ cell_x + cell_y * hierarchy_level.size[0] ];

	if( level == 0u )
	{
		cell_value= 0xFFFFu;
		return;
	}

	for( unsigned int subcell_y= 0u; subcell_y < 4u; subcell_y++ )
	for( unsigned int subcell_x= 0u; subcell_x < 4u; subcell_x++ )
	{
		const unsigned int bit_number= subcell_x + ( subcell_y << 2u );
		const unsigned int bit_mask= 1u << bit_number;
		if( ( cell_value & bit_mask ) != 0u )
			continue;

		SetToOneOcclusionHierarchyCell_r< level == 0u ? 0u : level - 1u >(
			cell_x * 4u + subcell_x,
			cell_y * 4u + subcell_y );
	}

	cell_value= 0xFFFFu;
}

void Rasterizer::UpdateOcclusionHierarchy(
	const RasterizerVertex* const polygon_vertices, const unsigned int polygon_vertex_count,
	const bool has_alpha )
{
	constexpr unsigned int c_max_vertices= 32u;
	PC_ASSERT( polygon_vertex_count < c_max_vertices );
	PC_ASSERT( polygon_vertex_count >= 3u );

	fixed16_t x_min= viewport_size_x_ << 16, x_max= 0;
	fixed16_t y_min= viewport_size_y_ << 16, y_max= 0;

	for( unsigned int v= 0u; v < polygon_vertex_count; v++ )
	{
		if( polygon_vertices[v].x < x_min ) x_min= polygon_vertices[v].x;
		if( polygon_vertices[v].x > x_max ) x_max= polygon_vertices[v].x;
		if( polygon_vertices[v].y < y_min ) y_min= polygon_vertices[v].y;
		if( polygon_vertices[v].y > y_max ) y_max= polygon_vertices[v].y;
	}
	PC_ASSERT( x_min <= x_max );
	PC_ASSERT( y_min <= y_max );

	const int x_min_i= std::max( 0, x_min >> 16 );
	const int x_max_i= std::min( ( x_max + g_fixed16_one ) >> 16, viewport_size_x_ );
	const int y_min_i= std::max( 0, y_min >> 16 );
	const int y_max_i= std::min( ( y_max + g_fixed16_one ) >> 16, viewport_size_y_ );
	const int x_delta= x_max_i - x_min_i;
	const int y_delta= y_max_i - y_min_i;
	const int max_delta= std::max( x_delta, y_delta );

	// TODO - calibrate hierarchy level selection.
	int hierarchy_level= 0;
	while( (max_delta >> 4) > ( 16 << (hierarchy_level*2) ) )
		hierarchy_level++;

	hierarchy_level= std::min( hierarchy_level, int(c_occlusion_hierarchy_levels) - 1 );

	const int cell_size_log2= 4 + (hierarchy_level*2);
	const int cell_size= 1 << cell_size_log2;

	const int cell_x_min= x_min_i >> cell_size_log2;
	const int cell_x_max= ( x_max_i + ( cell_size - 1 ) ) >> cell_size_log2;
	const int cell_y_min= y_min_i >> cell_size_log2;
	const int cell_y_max= ( y_max_i + ( cell_size - 1 ) ) >> cell_size_log2;

	unsigned int (Rasterizer::*update_cell_func)( unsigned int, unsigned int );
	void (Rasterizer::*set_one_cell_func)( unsigned int, unsigned int );
	switch(hierarchy_level)
	{
	case 0:
		update_cell_func= &Rasterizer::UpdateOcclusionHierarchyCell_r<0>;
		set_one_cell_func= &Rasterizer::SetToOneOcclusionHierarchyCell_r<0>;
		break;
	case 1:
		update_cell_func= &Rasterizer::UpdateOcclusionHierarchyCell_r<1>;
		set_one_cell_func= &Rasterizer::SetToOneOcclusionHierarchyCell_r<1>;
		break;
	case 2:
		update_cell_func= &Rasterizer::UpdateOcclusionHierarchyCell_r<2>;
		set_one_cell_func= &Rasterizer::SetToOneOcclusionHierarchyCell_r<2>;
		break;
	default:
		PC_ASSERT(false); update_cell_func= nullptr; set_one_cell_func= nullptr;
		break;
	};

	// For small polygons or polygons with alpha just update whole polygon bounding box.
	if( has_alpha || (max_delta >> 4) < 8 )
	{
		// Update cells of selected level recursively from upper to lower.
		for( int cell_y= cell_y_min; cell_y < cell_y_max; cell_y++ )
		for( int cell_x= cell_x_min; cell_x < cell_x_max; cell_x++ )
			(this->*update_cell_func)( cell_x, cell_y );
	}
	else
	{
		// For levels > 0 test cells inside polygon for fast accept/reject.
		// If cell is fully inside polygon - set this cell and low levels related cells to one.
		// If cell is fully outside polygon - reject if.
		// Else - make deep hierarchy update.

		fixed16_t normals[c_max_vertices][2], center[2]= { 0, 0 };
		for( unsigned int i= 0u; i < polygon_vertex_count; i++ )
		{
			const unsigned int next_i= ( i == polygon_vertex_count - 1u ) ? 0u : (i+1u);
			const float dx= ( polygon_vertices[next_i].x - polygon_vertices[i].x ) / 65536.0f;
			const float dy= ( polygon_vertices[next_i].y - polygon_vertices[i].y ) / 65536.0f;
			const float length= std::sqrt( dx * dx + dy * dy );
			normals[i][0]= -fixed16_t( 65536.0f * float(dy) / length );
			normals[i][1]= +fixed16_t( 65536.0f * float(dx) / length );

			center[0]+= polygon_vertices[i].x;
			center[1]+= polygon_vertices[i].y;
		}
		center[0]/= polygon_vertex_count;
		center[1]/= polygon_vertex_count;

		// Reverse normals, if they directed from polygon center.
		fixed16_t center_vec[2];
		center_vec[0]= center[0]- polygon_vertices[0].x;
		center_vec[1]= center[1]- polygon_vertices[0].y;
		if( Fixed16Mul( center_vec[0], normals[0][0] ) + Fixed16Mul( center_vec[1], normals[0][1] ) < 0 )
		{
			for( unsigned int i= 0u; i < polygon_vertex_count; i++ )
			{
				normals[i][0]= -normals[i][0];
				normals[i][1]= -normals[i][1];
			}
		}

		const fixed16_t cell_outer_radius= ( cell_size << 16 ) * 3 / 4; // Radius of quad - side * sqrt(2) / 2 ~ 0.7. Use close number.
		const fixed16_t half_cell_size= 1 << ( cell_size_log2 + 16 - 1 );

		for( int cell_y= cell_y_min; cell_y < cell_y_max; cell_y++ )
		for( int cell_x= cell_x_min; cell_x < cell_x_max; cell_x++ )
		{
			const fixed16_t cell_center_x= ( cell_x << ( cell_size_log2 + 16 ) ) + half_cell_size;
			const fixed16_t cell_center_y= ( cell_y << ( cell_size_log2 + 16 ) ) + half_cell_size;

			int status= 1; // -1 outside, 0 mixed, 1 - inside
			for( unsigned int v= 0u; v < polygon_vertex_count; v++ )
			{
				fixed16_t vec[2];
				vec[0]= cell_center_x - polygon_vertices[v].x;
				vec[1]= cell_center_y - polygon_vertices[v].y;
				const fixed16_t signed_distance= Fixed16Mul( normals[v][0], vec[0] ) + Fixed16Mul( normals[v][1], vec[1] );
				if( signed_distance < -cell_outer_radius )
				{
					status= -1;
					break;
				}
				else if( signed_distance > cell_outer_radius ) {}
				else
				{
					status= 0;
					break;
				}
			}

			if( status == 0 )
				(this->*update_cell_func)( cell_x, cell_y );
			else if( status == 1 )
				(this->*set_one_cell_func)( cell_x, cell_y );
			else // if( status == -1 )
				continue;
		}
	}

	// Update cells from lower to upper.
	for( int i= hierarchy_level + 1; i < int(c_occlusion_hierarchy_levels); i++ )
	{
		const int cell_size_log2= 4 + (i*2);
		const int cell_size= 1 << cell_size_log2;

		const int cell_x_min= x_min_i >> cell_size_log2;
		const int cell_x_max= ( x_max_i + ( cell_size - 1 ) ) >> cell_size_log2;
		const int cell_y_min= y_min_i >> cell_size_log2;
		const int cell_y_max= ( y_max_i + ( cell_size - 1 ) ) >> cell_size_log2;

		const auto& level= occlusion_hierarchy_levels_[i];
		const auto& level_minus_one= occlusion_hierarchy_levels_[ i - 1 ];

		for( int cell_y= cell_y_min; cell_y < cell_y_max; cell_y++ )
		for( int cell_x= cell_x_min; cell_x < cell_x_max; cell_x++ )
		{
			PC_ASSERT( cell_x < int(level.size[0]) );
			PC_ASSERT( cell_y < int(level.size[1]) );
			unsigned short& cell_value= level.data[ cell_x + cell_y * int(level.size[0]) ];
			if( cell_value == 0xFFFFu )
				continue;

			for( int subcell_y= 0; subcell_y < 4; subcell_y++ )
			for( int subcell_x= 0; subcell_x < 4; subcell_x++ )
			{
				PC_ASSERT( cell_x * 4 + subcell_x < int(level_minus_one.size[0]) );
				PC_ASSERT( cell_y * 4 + subcell_y < int(level_minus_one.size[1]) );

				const int bit_number= subcell_x + ( subcell_y << 2 );
				const unsigned short lower_value=
					level_minus_one.data[
						  cell_x * 4 + subcell_x +
						( cell_y * 4 + subcell_y ) * int(level_minus_one.size[0]) ];

				if( lower_value == 0xFFFFu )
					cell_value|= 1u << bit_number;
			}
		}
	} // for upper levels
}

bool Rasterizer::IsOccluded(
	const RasterizerVertex* const polygon_vertices, const unsigned int polygon_vertex_count ) const
{
	PC_ASSERT( polygon_vertex_count >= 3u );

	fixed16_t x_min= viewport_size_x_ << 16, x_max= 0;
	fixed16_t y_min= viewport_size_y_ << 16, y_max= 0;

	for( unsigned int v= 0u; v < polygon_vertex_count; v++ )
	{
		if( polygon_vertices[v].x < x_min ) x_min= polygon_vertices[v].x;
		if( polygon_vertices[v].x > x_max ) x_max= polygon_vertices[v].x;
		if( polygon_vertices[v].y < y_min ) y_min= polygon_vertices[v].y;
		if( polygon_vertices[v].y > y_max ) y_max= polygon_vertices[v].y;
	}
	PC_ASSERT( x_min <= x_max );
	PC_ASSERT( y_min <= y_max );

	const int x_min_i= std::max( 0, x_min >> 16 );
	const int x_max_i= std::min( ( x_max + g_fixed16_one ) >> 16, viewport_size_x_ );
	const int y_min_i= std::max( 0, y_min >> 16 );
	const int y_max_i= std::min( ( y_max + g_fixed16_one ) >> 16, viewport_size_y_ );
	const int x_delta= x_max_i - x_min_i;
	const int y_delta= y_max_i - y_min_i;
	const int max_delta= std::max( x_delta, y_delta );

	// TODO - calibrate hierarchy level selection.
	int hierarchy_level= 0;
	while( (max_delta >> 3) > ( 16 << (hierarchy_level*2) ) )
		hierarchy_level++;

	// For small polygons test separate bits of level 0.
	// TODO - maybe for upper levels test not whole 16bit word, but separate bits of level+1 ?
	if( (max_delta >> 3) <= 8 )
	{
		const auto& level= occlusion_hierarchy_levels_[0];

		const int subcell_x_min= x_min_i >> 2;
		const int subcell_x_max= ( x_max_i + 3 ) >> 2;
		const int subcell_y_min= y_min_i >> 2;
		const int subcell_y_max= ( y_max_i + 3 ) >> 2;
		PC_ASSERT( subcell_x_max <= int(level.size[0]) * 4 );
		PC_ASSERT( subcell_y_max <= int(level.size[1]) * 4 );

		for( int subcell_y= subcell_y_min; subcell_y < subcell_y_max; subcell_y++ )
		for( int subcell_x= subcell_x_min; subcell_x < subcell_x_max; subcell_x++ )
		{
			const int cell_x= subcell_x >> 2;
			const int cell_y= subcell_y >> 2;
			const int bit_x= subcell_x & 3;
			const int bit_y= subcell_y & 3;
			const int bit_mask= 1 << ( bit_x + 4 * bit_y );

			if( ( level.data[ cell_x + cell_y * int(level.size[0]) ] & bit_mask ) == 0 )
				return false;
		}
		return true;
	}

	hierarchy_level= std::min( hierarchy_level, int(c_occlusion_hierarchy_levels) - 1 );

	const auto& level= occlusion_hierarchy_levels_[hierarchy_level];

	const int cell_size_log2= 4 + (hierarchy_level*2);
	const int cell_size= 1 << cell_size_log2;

	const int cell_x_min= x_min_i >> cell_size_log2;
	const int cell_x_max= ( x_max_i + ( cell_size - 1 ) ) >> cell_size_log2;
	const int cell_y_min= y_min_i >> cell_size_log2;
	const int cell_y_max= ( y_max_i + ( cell_size - 1 ) ) >> cell_size_log2;

	PC_ASSERT( cell_x_max <= int(level.size[0]) );
	PC_ASSERT( cell_y_max <= int(level.size[1]) );

	for( int cell_y= cell_y_min; cell_y < cell_y_max; cell_y++ )
	for( int cell_x= cell_x_min; cell_x < cell_x_max; cell_x++ )
	{
		if( level.data[ cell_x + cell_y * int(level.size[0]) ] != 0xFFFFu )
			return false;
	}

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
	unsigned int level= (1u + tick_count) % ( c_occlusion_hierarchy_levels + 2u );

	if( level == 0u )
		return;
	else if( level == 1u )
	{
		for( int y= 0u; y < viewport_size_y_; y++ )
		for( int x= 0u; x < viewport_size_x_; x++ )
			color_buffer_[ x + y * row_size_ ]=
				( occlusion_buffer_[ (x>>3) + y * occlusion_buffer_width_ ] & (1<<(x&7)) ) == 0u
					? 0x00000000u
					: 0xFFFFFFFFu;
	}
	else
	{
		level-= 2u;
		const auto& hierarchy_level= occlusion_hierarchy_levels_[level];
		const unsigned int cell_size= 16u << (2u * level);
		const unsigned int cell_bit_size= cell_size / 4u;

		for( unsigned int y= 0u; y < static_cast<unsigned int>(viewport_size_y_); y++ )
		for( unsigned int x= 0u; x < static_cast<unsigned int>(viewport_size_x_); x++ )
		{
			const unsigned int cell_x= x / cell_size;
			const unsigned int cell_y= y / cell_size;
			const unsigned short cell_value= hierarchy_level.data[ cell_x + cell_y * hierarchy_level.size[0] ];
			const unsigned int bit_x= ( x - cell_x * cell_size ) / cell_bit_size;
			const unsigned int bit_y= ( y - cell_y * cell_size ) / cell_bit_size;
			const unsigned int bit_mask= 1u << ( bit_x + 4u * bit_y );

			color_buffer_[ x + y * row_size_ ]= (cell_value & bit_mask) == 0 ? 0x00000000u : 0xFFFFFFFFu;
		}
	}
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

void Rasterizer::SetLight( const fixed16_t light )
{
	light_= light;
}

void Rasterizer::DrawAffineColoredTriangle( const RasterizerVertex* const vertices, const uint32_t color )
{
	PC_ASSERT( vertices[0].z > ( g_fixed16_one >> c_max_inv_z_min_log2 ) );
	PC_ASSERT( vertices[1].z > ( g_fixed16_one >> c_max_inv_z_min_log2 ) );
	PC_ASSERT( vertices[2].z > ( g_fixed16_one >> c_max_inv_z_min_log2 ) );

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

	const fixed16_t middle_lower_dy= vertices[ middle_index ].y - vertices[ lower_index ].y;
	const fixed16_t upper_middle_dy= vertices[ upper_index ].y - vertices[ middle_index ].y;
	// TODO - optimize "triangle_part_vertices_" assignment.
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
		triangle_part_x_step_right_= Fixed16Div( vertices[ upper_index ].x - vertices[ lower_index ].x, long_edge_y_length );

		if( middle_lower_dy > 0 )
		{
			triangle_part_x_step_left_= Fixed16Div( vertices[ middle_index ].x - vertices[ lower_index ].x, middle_lower_dy );
			triangle_part_vertices_[0]= vertices[ lower_index ];
			triangle_part_vertices_[1]= vertices[ middle_index ];
			triangle_part_vertices_[2]= vertices[ lower_index ];
			triangle_part_vertices_[3]= vertices[ upper_index ];
			DrawAffineColoredTrianglePart( color );
		}
		if( upper_middle_dy > 0 )
		{
			triangle_part_x_step_left_= Fixed16Div( vertices[ upper_index ].x - vertices[ middle_index ].x, upper_middle_dy );
			triangle_part_vertices_[0]= vertices[ middle_index ];
			triangle_part_vertices_[1]= vertices[ upper_index ];
			triangle_part_vertices_[2]= vertices[ lower_index ];
			triangle_part_vertices_[3]= vertices[ upper_index ];
			DrawAffineColoredTrianglePart( color );
		}
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
		triangle_part_x_step_left_= Fixed16Div( vertices[ upper_index ].x - vertices[ lower_index ].x, long_edge_y_length );

		if( middle_lower_dy > 0 )
		{
			triangle_part_x_step_right_= Fixed16Div( vertices[ middle_index ].x - vertices[ lower_index ].x, middle_lower_dy );
			triangle_part_vertices_[0]= vertices[ lower_index ];
			triangle_part_vertices_[1]= vertices[ upper_index ];
			triangle_part_vertices_[2]= vertices[ lower_index ];
			triangle_part_vertices_[3]= vertices[ middle_index ];
			DrawAffineColoredTrianglePart( color );
		}
		if( upper_middle_dy > 0 )
		{
			triangle_part_x_step_right_= Fixed16Div( vertices[ upper_index ].x - vertices[ middle_index ].x, upper_middle_dy );
			triangle_part_vertices_[0]= vertices[ lower_index ];
			triangle_part_vertices_[1]= vertices[ upper_index ];
			triangle_part_vertices_[2]= vertices[ middle_index ];
			triangle_part_vertices_[3]= vertices[ upper_index ];
			DrawAffineColoredTrianglePart( color );
		}
	}
}

void Rasterizer::DrawColoredConvexPolygon(
	const RasterizerVertex* const vertices,
	const unsigned int vertex_count,
	const bool is_anticlockwise,
	const uint32_t color )
{
	PC_ASSERT( vertex_count >= 3u && vertex_count <= c_max_polygon_vertices );

	fixed16_t y_min= vertices[0].y, y_max= vertices[0].y;
	unsigned int lower_vertex_index= 0u, upper_vertex_index= 0u;
	for( unsigned int v= 0u; v < vertex_count; v++ )
	{
		if( vertices[v].y < y_min )
		{
			y_min= vertices[v].y;
			lower_vertex_index= v;
		}
		if( vertices[v].y > y_max )
		{
			y_max= vertices[v].y;
			upper_vertex_index= v;
		}
	}

	if( lower_vertex_index == upper_vertex_index )
		return; // Degenerate polygon.

	const auto next_index=
	[vertex_count]( const unsigned int v ) -> unsigned int
	{
		unsigned int result= v + 1u;
		if( result >= vertex_count ) result-= vertex_count;
		return result;
	};
	const auto prev_index=
	[vertex_count]( const unsigned int v ) -> unsigned int
	{
		return v == 0u ? ( vertex_count - 1u ) : ( v - 1u );
	};

	unsigned int left_index = lower_vertex_index;
	unsigned int right_index= lower_vertex_index;
	for( unsigned int v= 1u; v < vertex_count; v++ )
	{
		unsigned int more_upper_left, more_upper_right;
		if( is_anticlockwise )
		{
			more_upper_left = prev_index( left_index  );
			more_upper_right= next_index( right_index );
		}
		else

		{
			more_upper_left = next_index( left_index  );
			more_upper_right= prev_index( right_index );
		}

		const fixed16_t dy_left = vertices[ more_upper_left  ].y - vertices[ left_index  ].y;
		const fixed16_t dy_right= vertices[ more_upper_right ].y - vertices[ right_index ].y;
		if( dy_left > 0 && dy_right > 0 )
		{
			triangle_part_x_step_left_ = Fixed16Div( vertices[ more_upper_left  ].x - vertices[ left_index  ].x, dy_left  );
			triangle_part_x_step_right_= Fixed16Div( vertices[ more_upper_right ].x - vertices[ right_index ].x, dy_right );
			triangle_part_vertices_[0]= vertices[ left_index  ];
			triangle_part_vertices_[1]= vertices[ more_upper_left  ];
			triangle_part_vertices_[2]= vertices[ right_index ];
			triangle_part_vertices_[3]= vertices[ more_upper_right ];
			DrawAffineColoredTrianglePart( color );
		}

		if( vertices[ more_upper_left ].y < vertices[ more_upper_right ].y )
			left_index = more_upper_left ;
		else
			right_index= more_upper_right;
	}
}

void Rasterizer::DrawShadowTriangle( const RasterizerVertex* const vertices )
{
	PC_ASSERT( vertices[0].z > ( g_fixed16_one >> c_max_inv_z_min_log2 ) );
	PC_ASSERT( vertices[1].z > ( g_fixed16_one >> c_max_inv_z_min_log2 ) );
	PC_ASSERT( vertices[2].z > ( g_fixed16_one >> c_max_inv_z_min_log2 ) );

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

	RasterizerVertexCoord middle_vertex;
	middle_vertex.x= middle_x;
	middle_vertex.y= vertices[ middle_index ].y;

	const fixed_base_t middle_inv_z_scaled=
			Fixed16Mul( upper_vertex_inv_z_scaled, middle_k ) +
			Fixed16Mul( lower_vertex_inv_z_scaled, ( g_fixed16_one - middle_k ) );

	const fixed16_t middle_lower_dy= vertices[ middle_index ].y - vertices[ lower_index ].y;
	const fixed16_t upper_middle_dy= vertices[ upper_index ].y - vertices[ middle_index ].y;
	// TODO - optimize "triangle_part_vertices_" assignment.
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
		const fixed16_t dx= middle_vertex.x - vertices[ middle_index ].x;
		if( dx <= 0 ) return;

		line_inv_z_scaled_step_= Fixed16Div( middle_inv_z_scaled - middle_vertex_inv_z_scaled, dx );

		triangle_part_x_step_right_= Fixed16Div( vertices[ upper_index ].x - vertices[ lower_index ].x, long_edge_y_length );
		if( middle_lower_dy > 0 )
		{
			triangle_part_x_step_left_= Fixed16Div( vertices[ middle_index ].x - vertices[ lower_index ].x, middle_lower_dy );
			triangle_part_inv_z_scaled_step_left_= Fixed16Div( middle_vertex_inv_z_scaled - lower_vertex_inv_z_scaled, middle_lower_dy );

			triangle_part_vertices_[0]= vertices[ lower_index ];
			triangle_part_vertices_[1]= vertices[ middle_index ];
			triangle_part_vertices_[2]= vertices[ lower_index ];
			triangle_part_vertices_[3]= vertices[ upper_index ];
			triangle_part_inv_z_scaled_left_= lower_vertex_inv_z_scaled;
			DrawShadowTrianglePart();
		}
		if( upper_middle_dy > 0 )
		{
			triangle_part_x_step_left_= Fixed16Div( vertices[ upper_index ].x - vertices[ middle_index ].x, upper_middle_dy );
			triangle_part_inv_z_scaled_step_left_= Fixed16Div( upper_vertex_inv_z_scaled - middle_vertex_inv_z_scaled, upper_middle_dy );

			triangle_part_vertices_[0]= vertices[ middle_index ];
			triangle_part_vertices_[1]= vertices[ upper_index ];
			triangle_part_vertices_[2]= vertices[ lower_index ];
			triangle_part_vertices_[3]= vertices[ upper_index ];
			triangle_part_inv_z_scaled_left_= middle_vertex_inv_z_scaled;
			DrawShadowTrianglePart();
		}
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
		const fixed16_t dx= vertices[ middle_index ].x - middle_vertex.x;
		if( dx <= 0 ) return;
		line_inv_z_scaled_step_= Fixed16Div( middle_vertex_inv_z_scaled - middle_inv_z_scaled, dx );

		triangle_part_x_step_left_= Fixed16Div( vertices[ upper_index ].x - vertices[ lower_index ].x, long_edge_y_length );
		triangle_part_inv_z_scaled_step_left_= Fixed16Div( upper_vertex_inv_z_scaled - lower_vertex_inv_z_scaled, long_edge_y_length );

		triangle_part_inv_z_scaled_left_= lower_vertex_inv_z_scaled;

		if( middle_lower_dy > 0 )
		{
			triangle_part_x_step_right_= Fixed16Div( vertices[ middle_index ].x - vertices[ lower_index ].x, middle_lower_dy );
			triangle_part_vertices_[0]= vertices[ lower_index ];
			triangle_part_vertices_[1]= vertices[ upper_index ];
			triangle_part_vertices_[2]= vertices[ lower_index ];
			triangle_part_vertices_[3]= vertices[ middle_index ];
			DrawShadowTrianglePart();
		}
		if( upper_middle_dy > 0 )
		{
			triangle_part_x_step_right_= Fixed16Div( vertices[ upper_index ].x - vertices[ middle_index ].x, upper_middle_dy );
			triangle_part_vertices_[0]= vertices[ lower_index ];
			triangle_part_vertices_[1]= vertices[ upper_index ];
			triangle_part_vertices_[2]= vertices[ middle_index ];
			triangle_part_vertices_[3]= vertices[ upper_index ];
			DrawShadowTrianglePart();
		}
	}
}


void Rasterizer::DrawAffineColoredTrianglePart( const uint32_t color )
{
	const fixed16_t y_start_f= std::max( triangle_part_vertices_[0].y, triangle_part_vertices_[2].y );
	const fixed16_t y_end_f  = std::min( triangle_part_vertices_[1].y, triangle_part_vertices_[3].y );
	const int y_start= std::max( 0, Fixed16RoundToInt( y_start_f ) );
	const int y_end  = std::min( viewport_size_y_, Fixed16RoundToInt( y_end_f ) );

	const fixed16_t y_cut_left = ( y_start << 16 ) + g_fixed16_half - triangle_part_vertices_[0].y;
	const fixed16_t y_cut_right= ( y_start << 16 ) + g_fixed16_half - triangle_part_vertices_[2].y;
	fixed16_t x_left = triangle_part_vertices_[0].x + Fixed16Mul( y_cut_left , triangle_part_x_step_left_  );
	fixed16_t x_right= triangle_part_vertices_[2].x + Fixed16Mul( y_cut_right, triangle_part_x_step_right_ );
	for(
		int y= y_start;
		y< y_end;
		y++,
		x_left += triangle_part_x_step_left_ ,
		x_right+= triangle_part_x_step_right_ )
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

void Rasterizer::DrawShadowTrianglePart()
{
	const fixed16_t y_start_f= std::max( triangle_part_vertices_[0].y, triangle_part_vertices_[2].y );
	const fixed16_t y_end_f  = std::min( triangle_part_vertices_[1].y, triangle_part_vertices_[3].y );
	const int y_start= std::max( 0, Fixed16RoundToInt( y_start_f ) );
	const int y_end  = std::min( viewport_size_y_, Fixed16RoundToInt( y_end_f ) );

	const fixed16_t y_cut_left = ( y_start << 16 ) + g_fixed16_half - triangle_part_vertices_[0].y;
	const fixed16_t y_cut_right= ( y_start << 16 ) + g_fixed16_half - triangle_part_vertices_[2].y;
	fixed16_t x_left = triangle_part_vertices_[0].x + Fixed16Mul( y_cut_left , triangle_part_x_step_left_  );
	fixed16_t x_right= triangle_part_vertices_[2].x + Fixed16Mul( y_cut_right, triangle_part_x_step_right_ );
	fixed_base_t inv_z_scaled_left= triangle_part_inv_z_scaled_left_ + Fixed16Mul( y_cut_left, triangle_part_inv_z_scaled_step_left_ );

	for(
		int y= y_start;
		y< y_end;
		y++,
		x_left += triangle_part_x_step_left_ ,
		x_right+= triangle_part_x_step_right_,
		inv_z_scaled_left+= triangle_part_inv_z_scaled_step_left_ )
	{
		const int x_start= std::max( 0, Fixed16RoundToInt( x_left ) );
		const int x_end= std::min( viewport_size_x_, Fixed16RoundToInt( x_right ) );
		const fixed16_t x_cut= ( x_start << 16 ) + g_fixed16_half - x_left;

		fixed_base_t line_inv_z_scaled= inv_z_scaled_left + Fixed16Mul( x_cut, line_inv_z_scaled_step_ );

		uint32_t* const dst= color_buffer_ + y * row_size_;
		const unsigned short* const depth_dst= depth_buffer_ + y * depth_buffer_width_;

		for( int x= x_start; x < x_end; x++,line_inv_z_scaled+= line_inv_z_scaled_step_ )
		{
			const unsigned short depth= line_inv_z_scaled >> ( c_inv_z_scaler_log2 + c_max_inv_z_min_log2 );
			if( depth > depth_dst[x] )
				dst[x]= ( dst[x] & 0xFEFEFEFEu ) >> 1u;
		}
	} // for y
}

} // namespace PanzerChasm
