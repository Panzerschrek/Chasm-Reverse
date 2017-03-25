#pragma once
#include "rasterizer.hpp"

namespace PanzerChasm
{

template< class TrianglePartDrawFunc, TrianglePartDrawFunc func>
void Rasterizer::DrawTrianglePerspectiveCorrectedImpl( const RasterizerVertex* vertices )
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
	RasterizerVertexTexCoord  lower_vertex_tc_div_z;
	RasterizerVertexTexCoord middle_vertex_tc_div_z;
	RasterizerVertexTexCoord  upper_vertex_tc_div_z;
	 lower_vertex_tc_div_z.u= Fixed16Div( vertices[ lower_index  ].u, vertices[ lower_index  ].z );
	 lower_vertex_tc_div_z.v= Fixed16Div( vertices[ lower_index  ].v, vertices[ lower_index  ].z );
	middle_vertex_tc_div_z.u= Fixed16Div( vertices[ middle_index ].u, vertices[ middle_index ].z );
	middle_vertex_tc_div_z.v= Fixed16Div( vertices[ middle_index ].v, vertices[ middle_index ].z );
	 upper_vertex_tc_div_z.u= Fixed16Div( vertices[ upper_index  ].u, vertices[ upper_index  ].z );
	 upper_vertex_tc_div_z.v= Fixed16Div( vertices[ upper_index  ].v, vertices[ upper_index  ].z );


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

	RasterizerVertexTexCoord middle_vertex_tex_coord;
	middle_vertex_tex_coord.u=
		Fixed16Mul( upper_vertex_tc_div_z.u, middle_k ) +
		Fixed16Mul( lower_vertex_tc_div_z.u, ( g_fixed16_one - middle_k ) );
	middle_vertex_tex_coord.v=
		Fixed16Mul( upper_vertex_tc_div_z.v, middle_k ) +
		Fixed16Mul( lower_vertex_tc_div_z.v, ( g_fixed16_one - middle_k ) );

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
		const fixed16_t dx= middle_vertex.x - vertices[ middle_index ].x;
		if( dx <= 0 ) return;
		line_tc_step_[0]= Fixed16Div( middle_vertex_tex_coord.u - middle_vertex_tc_div_z.u, dx );
		line_tc_step_[1]= Fixed16Div( middle_vertex_tex_coord.v - middle_vertex_tc_div_z.v, dx );
		line_inv_z_scaled_step_= Fixed16Div( middle_inv_z_scaled - middle_vertex_inv_z_scaled, dx );

		triangle_part_vertices_[0]= vertices[ lower_index ];
		triangle_part_vertices_[1]= vertices[ middle_index ];
		triangle_part_vertices_[2]= vertices[ lower_index ];
		triangle_part_vertices_[3]= middle_vertex;
		triangle_part_tex_coords_[0]=  lower_vertex_tc_div_z;
		triangle_part_tex_coords_[1]= middle_vertex_tc_div_z;
		triangle_part_inv_z_scaled[0]= lower_vertex_inv_z_scaled;
		triangle_part_inv_z_scaled[1]= middle_vertex_inv_z_scaled;
		(this->*func)();

		triangle_part_vertices_[0]= vertices[ middle_index ];
		triangle_part_vertices_[1]= vertices[ upper_index ];
		triangle_part_vertices_[2]= middle_vertex;
		triangle_part_vertices_[3]= vertices[ upper_index ];
		triangle_part_tex_coords_[0]= middle_vertex_tc_div_z;
		triangle_part_tex_coords_[1]=  upper_vertex_tc_div_z;
		triangle_part_inv_z_scaled[0]= middle_vertex_inv_z_scaled;
		triangle_part_inv_z_scaled[1]= upper_vertex_inv_z_scaled;
		(this->*func)();
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
		line_tc_step_[0]= Fixed16Div( middle_vertex_tc_div_z.u - middle_vertex_tex_coord.u, dx );
		line_tc_step_[1]= Fixed16Div( middle_vertex_tc_div_z.v - middle_vertex_tex_coord.v, dx );
		line_inv_z_scaled_step_= Fixed16Div( middle_vertex_inv_z_scaled - middle_inv_z_scaled, dx );

		triangle_part_vertices_[0]= vertices[ lower_index ];
		triangle_part_vertices_[1]= middle_vertex;
		triangle_part_vertices_[2]= vertices[ lower_index ];
		triangle_part_vertices_[3]= vertices[ middle_index ];
		triangle_part_tex_coords_[0]= lower_vertex_tc_div_z;
		triangle_part_tex_coords_[1]= middle_vertex_tex_coord;
		triangle_part_inv_z_scaled[0]= lower_vertex_inv_z_scaled;
		triangle_part_inv_z_scaled[1]= middle_inv_z_scaled;
		(this->*func)();

		triangle_part_vertices_[0]= middle_vertex;
		triangle_part_vertices_[1]= vertices[ upper_index ];
		triangle_part_vertices_[2]= vertices[ middle_index ];
		triangle_part_vertices_[3]= vertices[ upper_index ];
		triangle_part_tex_coords_[0]= middle_vertex_tex_coord;
		triangle_part_tex_coords_[1]= upper_vertex_tc_div_z;
		triangle_part_inv_z_scaled[0]= middle_inv_z_scaled;
		triangle_part_inv_z_scaled[1]= upper_vertex_inv_z_scaled;
		(this->*func)();
	}
}

template<
	Rasterizer::DepthTest depth_test, Rasterizer::DepthWrite depth_write,
	Rasterizer::AlphaTest alpha_test,
	Rasterizer::OcclusionTest occlusion_test, Rasterizer::OcclusionWrite occlusion_write>
void Rasterizer::DrawTexturedTrianglePerLineCorrectedPart()
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

	fixed16_t d_tc_div_z_left[2], tc_div_z_left_step[2];
	d_tc_div_z_left[0]= triangle_part_tex_coords_[1].u - triangle_part_tex_coords_[0].u;
	d_tc_div_z_left[1]= triangle_part_tex_coords_[1].v - triangle_part_tex_coords_[0].v;
	tc_div_z_left_step[0]= Fixed16Div( d_tc_div_z_left[0], dy );
	tc_div_z_left_step[1]= Fixed16Div( d_tc_div_z_left[1], dy );

	const fixed_base_t d_inv_z_scaled_left= triangle_part_inv_z_scaled[1] - triangle_part_inv_z_scaled[0];
	const fixed_base_t inv_z_scaled_left_step= Fixed16Div( d_inv_z_scaled_left, dy );

	const int y_start= std::max( 0, Fixed16RoundToInt( triangle_part_vertices_[0].y ) );
	const int y_end  = std::min( viewport_size_y_, Fixed16RoundToInt( triangle_part_vertices_[1].y ) );

	const fixed16_t y_cut= ( y_start << 16 ) + g_fixed16_half - triangle_part_vertices_[0].y;
	fixed16_t x_left = triangle_part_vertices_[0].x + Fixed16Mul( y_cut, x_left_step  );
	fixed16_t x_right= triangle_part_vertices_[2].x + Fixed16Mul( y_cut, x_right_step );

	fixed16_t tc_div_z_left[2];
	tc_div_z_left[0]= triangle_part_tex_coords_[0].u + Fixed16Mul( y_cut, tc_div_z_left_step[0] );
	tc_div_z_left[1]= triangle_part_tex_coords_[0].v + Fixed16Mul( y_cut, tc_div_z_left_step[1] );

	fixed_base_t inv_z_scaled_left= triangle_part_inv_z_scaled[0] + Fixed16Mul( y_cut, inv_z_scaled_left_step );

	for(
		int y= y_start;
		y< y_end;
		y++,
		x_left+= x_left_step, x_right+= x_right_step,
		tc_div_z_left[0]+= tc_div_z_left_step[0], tc_div_z_left[1]+= tc_div_z_left_step[1],
		inv_z_scaled_left+= inv_z_scaled_left_step )
	{
		int x_start= std::max( 0, Fixed16RoundToInt( x_left ) );
		int x_end= std::min( viewport_size_x_, Fixed16RoundToInt( x_right ) );
		if( x_end <= x_start ) continue;

		uint8_t* occlusion_dst= occlusion_buffer_ + y * occlusion_buffer_width_;

		if( occlusion_test == OcclusionTest::Yes )
		{
			if( occlusion_dst[ x_start >> 3 ] == 0xFFu ) x_start= (x_start + 7) & (~7);
			while( x_start < x_end && occlusion_dst[ x_start >> 3 ] == 0xFFu ) x_start+= 8;
			if( occlusion_dst[ (x_end-1) >> 3 ] == 0xFFu ) x_end&= (~7);
			while( x_start < x_end && occlusion_dst[ (x_end-1) >> 3 ] == 0xFFu ) x_end-= 8;
			if( x_end <= x_start ) continue;
		}

		const int effective_dx= x_end - x_start - 1;
		const fixed16_t x_cut= ( x_start << 16 ) + g_fixed16_half - x_left;

		fixed16_t tc_div_z_start[2], tc_div_z_end[2], inv_z_scaled_start, inv_z_scaled_end, tc_start[2], tc_end[2], line_tc_step[2];

		tc_div_z_start[0]= tc_div_z_left[0] + Fixed16Mul( x_cut, line_tc_step_[0] );
		tc_div_z_start[1]= tc_div_z_left[1] + Fixed16Mul( x_cut, line_tc_step_[1] );

		inv_z_scaled_start= inv_z_scaled_left  + Fixed16Mul( x_cut, line_inv_z_scaled_step_ );
		if( inv_z_scaled_start < 0 ) inv_z_scaled_start= 0;

		tc_start[0]= FixedDiv< 16 + c_inv_z_scaler_log2 >( tc_div_z_start[0], inv_z_scaled_start );
		tc_start[1]= FixedDiv< 16 + c_inv_z_scaler_log2 >( tc_div_z_start[1], inv_z_scaled_start );

		if( tc_start[0] < 0 ) tc_start[0]= 0;
		if( tc_start[0] > max_valid_tc_u_ ) tc_start[0]= max_valid_tc_u_;
		if( tc_start[1] < 0 ) tc_start[1]= 0;
		if( tc_start[1] > max_valid_tc_v_ ) tc_start[1]= max_valid_tc_v_;

		if( effective_dx != 0 )
		{
			tc_div_z_end[0]= tc_div_z_start[0] + effective_dx * line_tc_step_[0];
			tc_div_z_end[1]= tc_div_z_start[1] + effective_dx * line_tc_step_[1];

			inv_z_scaled_end= inv_z_scaled_start + effective_dx * line_inv_z_scaled_step_;
			if( inv_z_scaled_end < 0 ) inv_z_scaled_end= 0;

			tc_end[0]= FixedDiv< 16 + c_inv_z_scaler_log2 >( tc_div_z_end[0], inv_z_scaled_end );
			tc_end[1]= FixedDiv< 16 + c_inv_z_scaler_log2 >( tc_div_z_end[1], inv_z_scaled_end );

			if( tc_end[0] < 0 ) tc_end[0]= 0;
			if( tc_end[0] > max_valid_tc_u_ ) tc_end[0]= max_valid_tc_u_;
			if( tc_end[1] < 0 ) tc_end[1]= 0;
			if( tc_end[1] > max_valid_tc_v_ ) tc_end[1]= max_valid_tc_v_;

			line_tc_step[0]= ( tc_end[0] - tc_start[0] ) / effective_dx;
			line_tc_step[1]= ( tc_end[1] - tc_start[1] ) / effective_dx;
		}
		else
			line_tc_step[0]= line_tc_step[1]= 0;

		fixed16_t line_tc[2], line_inv_z_scaled;
		line_tc[0]= tc_start[0];
		line_tc[1]= tc_start[1];
		line_inv_z_scaled= inv_z_scaled_left + Fixed16Mul( x_cut, line_inv_z_scaled_step_ );

		uint32_t* dst= color_buffer_ + y * row_size_;
		unsigned short* depth_dst= depth_buffer_ + y * depth_buffer_width_;

		for( int x= x_start; x < x_end; x++,
			line_tc[0]+= line_tc_step[0], line_tc[1]+= line_tc_step[1],
			line_inv_z_scaled+= line_inv_z_scaled_step_ )
		{
			if( occlusion_test == OcclusionTest::Yes &&
				( occlusion_dst[ x >> 3u ] & (1u<<(x&7u)) ) != 0u )
				continue;

			// TODO - check this.
			// "depth" must be 65536 when inv_z == ( 1 << c_max_inv_z_min_log2 )
			const unsigned short depth= line_inv_z_scaled >> ( c_inv_z_scaler_log2 + c_max_inv_z_min_log2 );

			if( depth_test == DepthTest::No || depth > depth_dst[x] )
			{
				if( depth_write == DepthWrite::Yes ) depth_dst[x]= depth;
				if( occlusion_write == OcclusionWrite::Yes ) occlusion_dst[ x >> 3u ] |= 1u << (x&7u);

				const int u= line_tc[0] >> 16;
				const int v= line_tc[1] >> 16;
				PC_ASSERT( u >= 0 && u < texture_size_x_ );
				PC_ASSERT( v >= 0 && v < texture_size_y_ );
				dst[x]= texture_data_[ u + v * texture_size_x_ ];
			}
		}
	} // for y
}

template<
	Rasterizer::DepthTest depth_test, Rasterizer::DepthWrite depth_write,
	Rasterizer::AlphaTest alpha_test,
	Rasterizer::OcclusionTest occlusion_test, Rasterizer::OcclusionWrite occlusion_write>
void Rasterizer::DrawTexturedTriangleSpanCorrectedPart()
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

	fixed16_t d_tc_div_z_left[2], tc_div_z_left_step[2];
	d_tc_div_z_left[0]= triangle_part_tex_coords_[1].u - triangle_part_tex_coords_[0].u;
	d_tc_div_z_left[1]= triangle_part_tex_coords_[1].v - triangle_part_tex_coords_[0].v;
	tc_div_z_left_step[0]= Fixed16Div( d_tc_div_z_left[0], dy );
	tc_div_z_left_step[1]= Fixed16Div( d_tc_div_z_left[1], dy );

	const fixed_base_t d_inv_z_scaled_left= triangle_part_inv_z_scaled[1] - triangle_part_inv_z_scaled[0];
	const fixed_base_t inv_z_scaled_left_step= Fixed16Div( d_inv_z_scaled_left, dy );

	const int y_start= std::max( 0, Fixed16RoundToInt( triangle_part_vertices_[0].y ) );
	const int y_end  = std::min( viewport_size_y_, Fixed16RoundToInt( triangle_part_vertices_[1].y ) );

	const fixed16_t y_cut= ( y_start << 16 ) + g_fixed16_half - triangle_part_vertices_[0].y;
	fixed16_t x_left = triangle_part_vertices_[0].x + Fixed16Mul( y_cut, x_left_step  );
	fixed16_t x_right= triangle_part_vertices_[2].x + Fixed16Mul( y_cut, x_right_step );

	fixed16_t tc_div_z_left[2];
	tc_div_z_left[0]= triangle_part_tex_coords_[0].u + Fixed16Mul( y_cut, tc_div_z_left_step[0] );
	tc_div_z_left[1]= triangle_part_tex_coords_[0].v + Fixed16Mul( y_cut, tc_div_z_left_step[1] );

	fixed_base_t inv_z_scaled_left= triangle_part_inv_z_scaled[0] + Fixed16Mul( y_cut, inv_z_scaled_left_step );

	for(
		int y= y_start;
		y< y_end;
		y++,
		x_left+= x_left_step, x_right+= x_right_step,
		tc_div_z_left[0]+= tc_div_z_left_step[0], tc_div_z_left[1]+= tc_div_z_left_step[1],
		inv_z_scaled_left+= inv_z_scaled_left_step )
	{
		int x_start= std::max( 0, Fixed16RoundToInt( x_left ) );
		int x_end= std::min( viewport_size_x_, Fixed16RoundToInt( x_right ) );
		if( x_end <= x_start ) continue;

		uint8_t* occlusion_dst= occlusion_buffer_ + y * occlusion_buffer_width_;

		if( occlusion_test == OcclusionTest::Yes )
		{
			if( occlusion_dst[ x_start >> 3 ] == 0xFFu ) x_start= (x_start + 7) & (~7);
			while( x_start < x_end && occlusion_dst[ x_start >> 3 ] == 0xFFu ) x_start+= 8;
			if( occlusion_dst[ (x_end-1) >> 3 ] == 0xFFu ) x_end&= (~7);
			while( x_start < x_end && occlusion_dst[ (x_end-1) >> 3 ] == 0xFFu ) x_end-= 8;
			if( x_end <= x_start ) continue;
		}

		uint32_t* dst= color_buffer_ + y * row_size_;
		unsigned short* depth_dst= depth_buffer_ + y * depth_buffer_width_;


		const fixed16_t x_cut= ( x_start << 16 ) + g_fixed16_half - x_left;
		fixed16_t tc_div_z_current[2], line_inv_z_scaled;
		line_inv_z_scaled= inv_z_scaled_left + Fixed16Mul( x_cut, line_inv_z_scaled_step_ );
		tc_div_z_current[0]= tc_div_z_left[0] + Fixed16Mul( x_cut, line_tc_step_[0] );
		tc_div_z_current[1]= tc_div_z_left[1] + Fixed16Mul( x_cut, line_tc_step_[1] );

		const int spans_x_start= ( x_start + c_z_correct_span_size_minus_one ) & (~c_z_correct_span_size_minus_one);
		const int spans_x_end= x_end & (~c_z_correct_span_size_minus_one);
		const int start_part_dx= std::min( spans_x_start, x_end ) - x_start;
		const int end_part_dx= x_end - spans_x_end;
		PC_ASSERT( start_part_dx >= 0 );
		PC_ASSERT( end_part_dx >= 0 );

		fixed16_t tc_current[2], tc_next[2], tc_step[2], span_tc[2];
		tc_current[0]= FixedDiv< 16 + c_inv_z_scaler_log2 >( tc_div_z_current[0], line_inv_z_scaled );
		tc_current[1]= FixedDiv< 16 + c_inv_z_scaler_log2 >( tc_div_z_current[1], line_inv_z_scaled );
		if( tc_current[0] < 0 ) tc_current[0]= 0;
		if( tc_current[0] > max_valid_tc_u_ ) tc_current[0]= max_valid_tc_u_;
		if( tc_current[1] < 0 ) tc_current[1]= 0;
		if( tc_current[1] > max_valid_tc_v_ ) tc_current[1]= max_valid_tc_v_;

		if( start_part_dx > 0 )
		{
			const fixed_base_t next_inv_z_scaled= line_inv_z_scaled + start_part_dx * line_inv_z_scaled_step_;
			tc_next[0]= FixedDiv< 16 + c_inv_z_scaler_log2 >( tc_div_z_current[0] + start_part_dx * line_tc_step_[0], next_inv_z_scaled );
			tc_next[1]= FixedDiv< 16 + c_inv_z_scaler_log2 >( tc_div_z_current[1] + start_part_dx * line_tc_step_[1], next_inv_z_scaled );
			if( tc_next[0] < 0 ) tc_next[0]= 0;
			if( tc_next[0] > max_valid_tc_u_ ) tc_next[0]= max_valid_tc_u_;
			if( tc_next[1] < 0 ) tc_next[1]= 0;
			if( tc_next[1] > max_valid_tc_v_ ) tc_next[1]= max_valid_tc_v_;

			tc_step[0]= ( tc_next[0] - tc_current[0] ) / start_part_dx;
			tc_step[1]= ( tc_next[1] - tc_current[1] ) / start_part_dx;
			span_tc[0]= tc_current[0];
			span_tc[1]= tc_current[1];

			// Draw start part here.
			for( int x= 0u; x < start_part_dx;
				x++, line_inv_z_scaled+= line_inv_z_scaled_step_,
				span_tc[0]+= tc_step[0], span_tc[1]+= tc_step[1] )
			{
				const int full_x= x_start + x;

				if( occlusion_test == OcclusionTest::Yes &&
					( occlusion_dst[ full_x >> 3 ] & (1<<(full_x&7)) ) != 0u )
					continue;

				const unsigned short depth= line_inv_z_scaled >> ( c_inv_z_scaler_log2 + c_max_inv_z_min_log2 );
				if( depth_test == DepthTest::No || depth > depth_dst[ full_x ] )
				{
					if( depth_write == DepthWrite::Yes ) depth_dst[ full_x ]= depth;
					if( occlusion_write == OcclusionWrite::Yes ) occlusion_dst[ full_x >> 3 ] |= 1 << (full_x&7);

					const int u= span_tc[0] >> 16;
					const int v= span_tc[1] >> 16;
					PC_ASSERT( u >= 0 && u < texture_size_x_ );
					PC_ASSERT( v >= 0 && v < texture_size_y_ );
					dst[ full_x ]= texture_data_[ u + v * texture_size_x_ ];
				}
			} // for span pixels

			line_inv_z_scaled= next_inv_z_scaled;
			tc_div_z_current[0]+= start_part_dx * line_tc_step_[0];
			tc_div_z_current[1]+= start_part_dx * line_tc_step_[1];
		}
		else
		{
			tc_next[0]= tc_current[0];
			tc_next[1]= tc_current[1];
		}

		for( int span_x= spans_x_start; span_x < spans_x_end;
			span_x+= c_z_correct_span_size,
			tc_div_z_current[0]+= line_tc_step_[0] << c_z_correct_span_size_log2,
			tc_div_z_current[1]+= line_tc_step_[1] << c_z_correct_span_size_log2 )
		{
			tc_current[0]= tc_next[0];
			tc_current[1]= tc_next[1];

			const fixed_base_t next_inv_z_scaled= line_inv_z_scaled + ( line_inv_z_scaled_step_ << c_z_correct_span_size_log2 );
			tc_next[0]= FixedDiv< 16 + c_inv_z_scaler_log2 >( tc_div_z_current[0] + ( line_tc_step_[0] << c_z_correct_span_size_log2 ), next_inv_z_scaled );
			tc_next[1]= FixedDiv< 16 + c_inv_z_scaler_log2 >( tc_div_z_current[1] + ( line_tc_step_[1] << c_z_correct_span_size_log2 ), next_inv_z_scaled );
			if( tc_next[0] < 0 ) tc_next[0]= 0;
			if( tc_next[0] > max_valid_tc_u_ ) tc_next[0]= max_valid_tc_u_;
			if( tc_next[1] < 0 ) tc_next[1]= 0;
			if( tc_next[1] > max_valid_tc_v_ ) tc_next[1]= max_valid_tc_v_;

			SpanOcclusionType occlusion_value= 0u;
			if( occlusion_test == OcclusionTest::Yes || occlusion_write == OcclusionWrite::Yes )
				occlusion_value= *reinterpret_cast<SpanOcclusionType*>(occlusion_dst + (span_x >> 3) );
			if( occlusion_test == OcclusionTest::Yes && occlusion_value == c_span_occlusion_value )
			{
				line_inv_z_scaled+= line_inv_z_scaled_step_ << c_z_correct_span_size_log2;
				continue;
			}

			tc_step[0]= ( tc_next[0] - tc_current[0] ) / c_z_correct_span_size;
			tc_step[1]= ( tc_next[1] - tc_current[1] ) / c_z_correct_span_size;
			span_tc[0]= tc_current[0];
			span_tc[1]= tc_current[1];

			for( int x= 0; x < c_z_correct_span_size;
				x++, line_inv_z_scaled+= line_inv_z_scaled_step_,
				span_tc[0]+= tc_step[0], span_tc[1]+= tc_step[1] )
			{
				if( occlusion_test == OcclusionTest::Yes &&
					( occlusion_value & ( 1 << x ) ) != 0 )
					continue;

				const unsigned short depth= line_inv_z_scaled >> ( c_inv_z_scaler_log2 + c_max_inv_z_min_log2 );
				if( depth_test == DepthTest::No || depth > depth_dst[ span_x + x ] )
				{
					if( depth_write == DepthWrite::Yes ) depth_dst[ span_x + x ]= depth;

					const int u= span_tc[0] >> 16;
					const int v= span_tc[1] >> 16;
					PC_ASSERT( u >= 0 && u < texture_size_x_ );
					PC_ASSERT( v >= 0 && v < texture_size_y_ );
					dst[ span_x + x ]= texture_data_[ u + v * texture_size_x_ ];
				}
			} // for span pixels

			if( occlusion_write == OcclusionWrite::Yes )
				*reinterpret_cast<SpanOcclusionType*>( occlusion_dst + (span_x >> 3) ) = c_span_occlusion_value;
		} // for spans

		if( end_part_dx > 0 && spans_x_start <= spans_x_end )
		{
			tc_current[0]= tc_next[0];
			tc_current[1]= tc_next[1];

			const fixed_base_t next_inv_z_scaled= line_inv_z_scaled + end_part_dx * line_inv_z_scaled_step_;
			tc_next[0]= FixedDiv< 16 + c_inv_z_scaler_log2 >( tc_div_z_current[0] + end_part_dx * line_tc_step_[0], next_inv_z_scaled );
			tc_next[1]= FixedDiv< 16 + c_inv_z_scaler_log2 >( tc_div_z_current[1] + end_part_dx * line_tc_step_[1], next_inv_z_scaled );
			if( tc_next[0] < 0 ) tc_next[0]= 0;
			if( tc_next[0] > max_valid_tc_u_ ) tc_next[0]= max_valid_tc_u_;
			if( tc_next[1] < 0 ) tc_next[1]= 0;
			if( tc_next[1] > max_valid_tc_v_ ) tc_next[1]= max_valid_tc_v_;

			tc_step[0]= ( tc_next[0] - tc_current[0] ) / end_part_dx;
			tc_step[1]= ( tc_next[1] - tc_current[1] ) / end_part_dx;
			span_tc[0]= tc_current[0];
			span_tc[1]= tc_current[1];

			// Draw end part here.
			for( int x= spans_x_end; x < x_end;
				x++, line_inv_z_scaled+= line_inv_z_scaled_step_,
				span_tc[0]+= tc_step[0], span_tc[1]+= tc_step[1] )
			{
				if( occlusion_test == OcclusionTest::Yes &&
					( occlusion_dst[ x >> 3 ] & (1<<(x&7)) ) != 0u )
					continue;

				const unsigned short depth= line_inv_z_scaled >> ( c_inv_z_scaler_log2 + c_max_inv_z_min_log2 );
				if( depth_test == DepthTest::No || depth > depth_dst[x] )
				{
					if( depth_write == DepthWrite::Yes ) depth_dst[x]= depth;
					if( occlusion_write == OcclusionWrite::Yes ) occlusion_dst[ x >> 3 ] |= 1 << (x&7);

					const int u= span_tc[0] >> 16;
					const int v= span_tc[1] >> 16;
					PC_ASSERT( u >= 0 && u < texture_size_x_ );
					PC_ASSERT( v >= 0 && v < texture_size_y_ );
					dst[x]= texture_data_[ u + v * texture_size_x_ ];
				}
			}
		}
	} // for y
}

template<
	Rasterizer::DepthTest depth_test, Rasterizer::DepthWrite depth_write,
	Rasterizer::AlphaTest alpha_test,
	Rasterizer::OcclusionTest occlusion_test, Rasterizer::OcclusionWrite occlusion_write>
void Rasterizer::DrawTexturedTrianglePerLineCorrected( const RasterizerVertex* vertices )
{
	Rasterizer::DrawTrianglePerspectiveCorrectedImpl<
		TrianglePartDrawFunc,
		&Rasterizer::DrawTexturedTrianglePerLineCorrectedPart<depth_test, depth_write, alpha_test, occlusion_test, occlusion_write > >
			( vertices );
}

template<
	Rasterizer::DepthTest depth_test, Rasterizer::DepthWrite depth_write,
	Rasterizer::AlphaTest alpha_test,
	Rasterizer::OcclusionTest occlusion_test, Rasterizer::OcclusionWrite occlusion_write>
void Rasterizer::DrawTexturedTriangleSpanCorrected( const RasterizerVertex* vertices )
{

	DrawTrianglePerspectiveCorrectedImpl<
		TrianglePartDrawFunc,
		&Rasterizer::DrawTexturedTriangleSpanCorrectedPart<depth_test, depth_write, alpha_test, occlusion_test, occlusion_write > >
			( vertices );
}

} // namespace PanzerChasm
