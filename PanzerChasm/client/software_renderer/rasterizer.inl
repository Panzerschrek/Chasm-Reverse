#pragma once
#include "rasterizer.hpp"

#ifdef PC_MMX_INSTRUCTIONS
#include <mmintrin.h>
#endif

namespace PanzerChasm
{

template<
	Rasterizer::DepthTest depth_test, Rasterizer::DepthWrite depth_write,
	Rasterizer::AlphaTest alpha_test,
	Rasterizer::OcclusionTest occlusion_test, Rasterizer::OcclusionWrite occlusion_write,
	Rasterizer::Lighting lighting, Rasterizer::Blending blending>
void Rasterizer::DrawAffineTexturedTriangle( const RasterizerVertex* vertices )
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

	RasterizerVertexTexCoord middle_vertex_tex_coord;
	middle_vertex_tex_coord.u=
		Fixed16Mul( vertices[ upper_index ].u, middle_k ) +
		Fixed16Mul( vertices[ lower_index ].u, ( g_fixed16_one - middle_k ) );
	middle_vertex_tex_coord.v=
		Fixed16Mul( vertices[ upper_index ].v, middle_k ) +
		Fixed16Mul( vertices[ lower_index ].v, ( g_fixed16_one - middle_k ) );

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
		line_tc_step_[0]= Fixed16Div( middle_vertex_tex_coord.u - vertices[ middle_index ].u, dx );
		line_tc_step_[1]= Fixed16Div( middle_vertex_tex_coord.v - vertices[ middle_index ].v, dx );
		line_inv_z_scaled_step_= Fixed16Div( middle_inv_z_scaled - middle_vertex_inv_z_scaled, dx );

		triangle_part_x_step_right_= Fixed16Div( vertices[ upper_index ].x - vertices[ lower_index ].x, long_edge_y_length );
		if( middle_lower_dy > 0 )
		{
			triangle_part_x_step_left_= Fixed16Div( vertices[ middle_index ].x - vertices[ lower_index ].x, middle_lower_dy );
			traingle_part_tc_step_left_[0]= Fixed16Div( vertices[ middle_index ].u - vertices[ lower_index ].u, middle_lower_dy );
			traingle_part_tc_step_left_[1]= Fixed16Div( vertices[ middle_index ].v - vertices[ lower_index ].v, middle_lower_dy );
			triangle_part_inv_z_scaled_step_left_= Fixed16Div( middle_vertex_inv_z_scaled - lower_vertex_inv_z_scaled, middle_lower_dy );

			triangle_part_vertices_[0]= vertices[ lower_index ];
			triangle_part_vertices_[1]= vertices[ middle_index ];
			triangle_part_vertices_[2]= vertices[ lower_index ];
			triangle_part_vertices_[3]= vertices[ upper_index ];
			trianlge_part_tc_left_= vertices[ lower_index ];
			triangle_part_inv_z_scaled_left_= lower_vertex_inv_z_scaled;
			DrawAffineTexturedTrianglePart< depth_test, depth_write, alpha_test, occlusion_test, occlusion_write, lighting, blending >();
		}
		if( upper_middle_dy > 0 )
		{
			triangle_part_x_step_left_= Fixed16Div( vertices[ upper_index ].x - vertices[ middle_index ].x, upper_middle_dy );
			traingle_part_tc_step_left_[0]= Fixed16Div( vertices[ upper_index ].u - vertices[ middle_index ].u, upper_middle_dy );
			traingle_part_tc_step_left_[1]= Fixed16Div( vertices[ upper_index ].v - vertices[ middle_index ].v, upper_middle_dy );
			triangle_part_inv_z_scaled_step_left_= Fixed16Div( upper_vertex_inv_z_scaled - middle_vertex_inv_z_scaled, upper_middle_dy );

			triangle_part_vertices_[0]= vertices[ middle_index ];
			triangle_part_vertices_[1]= vertices[ upper_index ];
			triangle_part_vertices_[2]= vertices[ lower_index ];
			triangle_part_vertices_[3]= vertices[ upper_index ];
			trianlge_part_tc_left_= vertices[ middle_index ];
			triangle_part_inv_z_scaled_left_= middle_vertex_inv_z_scaled;
			DrawAffineTexturedTrianglePart< depth_test, depth_write, alpha_test, occlusion_test, occlusion_write, lighting, blending >();
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
		line_tc_step_[0]= Fixed16Div( vertices[ middle_index ].u - middle_vertex_tex_coord.u, dx );
		line_tc_step_[1]= Fixed16Div( vertices[ middle_index ].v - middle_vertex_tex_coord.v, dx );
		line_inv_z_scaled_step_= Fixed16Div( middle_vertex_inv_z_scaled - middle_inv_z_scaled, dx );

		triangle_part_x_step_left_= Fixed16Div( vertices[ upper_index ].x - vertices[ lower_index ].x, long_edge_y_length );
		traingle_part_tc_step_left_[0]= Fixed16Div( vertices[ upper_index ].u - vertices[ lower_index ].u, long_edge_y_length );
		traingle_part_tc_step_left_[1]= Fixed16Div( vertices[ upper_index ].v - vertices[ lower_index ].v, long_edge_y_length );
		triangle_part_inv_z_scaled_step_left_= Fixed16Div( upper_vertex_inv_z_scaled - lower_vertex_inv_z_scaled, long_edge_y_length );

		trianlge_part_tc_left_= vertices[ lower_index ];
		triangle_part_inv_z_scaled_left_= lower_vertex_inv_z_scaled;

		if( middle_lower_dy > 0 )
		{
			triangle_part_x_step_right_= Fixed16Div( vertices[ middle_index ].x - vertices[ lower_index ].x, middle_lower_dy );
			triangle_part_vertices_[0]= vertices[ lower_index ];
			triangle_part_vertices_[1]= vertices[ upper_index ];
			triangle_part_vertices_[2]= vertices[ lower_index ];
			triangle_part_vertices_[3]= vertices[ middle_index ];
			DrawAffineTexturedTrianglePart< depth_test, depth_write, alpha_test, occlusion_test, occlusion_write, lighting, blending >();
		}
		if( upper_middle_dy > 0 )
		{
			triangle_part_x_step_right_= Fixed16Div( vertices[ upper_index ].x - vertices[ middle_index ].x, upper_middle_dy );
			triangle_part_vertices_[0]= vertices[ lower_index ];
			triangle_part_vertices_[1]= vertices[ upper_index ];
			triangle_part_vertices_[2]= vertices[ middle_index ];
			triangle_part_vertices_[3]= vertices[ upper_index ];
			DrawAffineTexturedTrianglePart< depth_test, depth_write, alpha_test, occlusion_test, occlusion_write, lighting, blending >();
		}
	}
}

template<Rasterizer::Lighting lighting>
inline uint32_t Rasterizer::ApplyLight( const uint32_t texel ) const
{
	if( lighting == Lighting::No )
		return texel;
	else
	{
		unsigned char components[4];
		for( unsigned int i= 0u; i < 3u; i++ )
		{
			const int c= ( reinterpret_cast<const unsigned char*>(&texel)[i] * light_ ) >> 16;
			components[i]= std::min( c, 255 );
		}
		components[3]= 0; // TODO - does this need?
		uint32_t result;
		std::memcpy( &result, components, sizeof(uint32_t) );
		return result;
	}
}

template<Rasterizer::Blending blending>
inline void Rasterizer::ApplyBlending( uint32_t& dst, uint32_t texel ) const
{
	if( blending == Blending::No )
		dst= texel;
	else
		dst= ( ( ( dst ^ texel ) & 0xFEFEFEFEu ) >> 1u ) + ( dst & texel );
}

template< class TrianglePartDrawFunc, TrianglePartDrawFunc func>
void Rasterizer::DrawTrianglePerspectiveCorrectedImpl( const RasterizerVertex* vertices )
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
		line_tc_step_[0]= Fixed16Div( middle_vertex_tex_coord.u - middle_vertex_tc_div_z.u, dx );
		line_tc_step_[1]= Fixed16Div( middle_vertex_tex_coord.v - middle_vertex_tc_div_z.v, dx );
		line_inv_z_scaled_step_= Fixed16Div( middle_inv_z_scaled - middle_vertex_inv_z_scaled, dx );

		triangle_part_x_step_right_= Fixed16Div( vertices[ upper_index ].x - vertices[ lower_index ].x, long_edge_y_length );
		if( middle_lower_dy > 0 )
		{
			triangle_part_x_step_left_= Fixed16Div( vertices[ middle_index ].x - vertices[ lower_index ].x, middle_lower_dy );
			traingle_part_tc_step_left_[0]= Fixed16Div( middle_vertex_tc_div_z.u - lower_vertex_tc_div_z.u, middle_lower_dy );
			traingle_part_tc_step_left_[1]= Fixed16Div( middle_vertex_tc_div_z.v - lower_vertex_tc_div_z.v, middle_lower_dy );
			triangle_part_inv_z_scaled_step_left_= Fixed16Div( middle_vertex_inv_z_scaled - lower_vertex_inv_z_scaled, middle_lower_dy );

			triangle_part_vertices_[0]= vertices[ lower_index ];
			triangle_part_vertices_[1]= vertices[ middle_index ];
			triangle_part_vertices_[2]= vertices[ lower_index ];
			triangle_part_vertices_[3]= vertices[ upper_index ];
			trianlge_part_tc_left_= lower_vertex_tc_div_z;
			triangle_part_inv_z_scaled_left_= lower_vertex_inv_z_scaled;
			(this->*func)();
		}
		if( upper_middle_dy > 0 )
		{
			triangle_part_x_step_left_= Fixed16Div( vertices[ upper_index ].x - vertices[ middle_index ].x, upper_middle_dy );
			traingle_part_tc_step_left_[0]= Fixed16Div( upper_vertex_tc_div_z.u - middle_vertex_tc_div_z.u, upper_middle_dy );
			traingle_part_tc_step_left_[1]= Fixed16Div( upper_vertex_tc_div_z.v - middle_vertex_tc_div_z.v, upper_middle_dy );
			triangle_part_inv_z_scaled_step_left_= Fixed16Div( upper_vertex_inv_z_scaled - middle_vertex_inv_z_scaled, upper_middle_dy );

			triangle_part_vertices_[0]= vertices[ middle_index ];
			triangle_part_vertices_[1]= vertices[ upper_index ];
			triangle_part_vertices_[2]= vertices[ lower_index ];
			triangle_part_vertices_[3]= vertices[ upper_index ];
			trianlge_part_tc_left_= middle_vertex_tc_div_z;
			triangle_part_inv_z_scaled_left_= middle_vertex_inv_z_scaled;
			(this->*func)();
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
		line_tc_step_[0]= Fixed16Div( middle_vertex_tc_div_z.u - middle_vertex_tex_coord.u, dx );
		line_tc_step_[1]= Fixed16Div( middle_vertex_tc_div_z.v - middle_vertex_tex_coord.v, dx );
		line_inv_z_scaled_step_= Fixed16Div( middle_vertex_inv_z_scaled - middle_inv_z_scaled, dx );

		triangle_part_x_step_left_= Fixed16Div( vertices[ upper_index ].x - vertices[ lower_index ].x, long_edge_y_length );
		traingle_part_tc_step_left_[0]= Fixed16Div( upper_vertex_tc_div_z.u - lower_vertex_tc_div_z.u, long_edge_y_length );
		traingle_part_tc_step_left_[1]= Fixed16Div( upper_vertex_tc_div_z.v - lower_vertex_tc_div_z.v, long_edge_y_length );
		triangle_part_inv_z_scaled_step_left_= Fixed16Div( upper_vertex_inv_z_scaled - lower_vertex_inv_z_scaled, long_edge_y_length );

		trianlge_part_tc_left_= lower_vertex_tc_div_z;
		triangle_part_inv_z_scaled_left_= lower_vertex_inv_z_scaled;

		if( middle_lower_dy > 0 )
		{
			triangle_part_x_step_right_= Fixed16Div( vertices[ middle_index ].x - vertices[ lower_index ].x, middle_lower_dy );
			triangle_part_vertices_[0]= vertices[ lower_index ];
			triangle_part_vertices_[1]= vertices[ upper_index ];
			triangle_part_vertices_[2]= vertices[ lower_index ];
			triangle_part_vertices_[3]= vertices[ middle_index ];
			(this->*func)();
		}
		if( upper_middle_dy > 0 )
		{
			triangle_part_x_step_right_= Fixed16Div( vertices[ upper_index ].x - vertices[ middle_index ].x, upper_middle_dy );
			triangle_part_vertices_[0]= vertices[ lower_index ];
			triangle_part_vertices_[1]= vertices[ upper_index ];
			triangle_part_vertices_[2]= vertices[ middle_index ];
			triangle_part_vertices_[3]= vertices[ upper_index ];
			(this->*func)();
		}
	}
}

template< class TrianglePartDrawFunc, TrianglePartDrawFunc func>
void Rasterizer::DrawConvexPolygonPerspectiveCorrectedImpl(
	const RasterizerVertex* const vertices,
	const unsigned int vertex_count,
	const bool is_anticlockwise )
{
	PC_ASSERT( vertex_count >= 3u );

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
	[vertex_count]( const unsigned int v )
	{
		unsigned int result= v + 1u;
		if( result >= vertex_count ) result-= vertex_count;
		return result;
	};
	const auto prev_index=
	[vertex_count]( const unsigned int v )
	{
		return v == 0u ? ( vertex_count - 1u ) : ( v - 1u );
	};

	RasterizerVertexTexCoord tex_coord_div_z[16];
	fixed_base_t inv_z_scaled[16];
	for( unsigned int v= 0u; v < vertex_count; v++ )
	{
		inv_z_scaled[v]= Fixed16Div( c_inv_z_scaler << 16, vertices[v].z );
		tex_coord_div_z[v].u= Fixed16Div( vertices[v].u, vertices[v].z );
		tex_coord_div_z[v].v= Fixed16Div( vertices[v].v, vertices[v].z );
	}

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
			traingle_part_tc_step_left_[0]= Fixed16Div( tex_coord_div_z[ more_upper_left  ].u - tex_coord_div_z[ left_index  ].u, dy_left  );
			traingle_part_tc_step_left_[1]= Fixed16Div( tex_coord_div_z[ more_upper_left  ].v - tex_coord_div_z[ left_index  ].v, dy_left  );
			triangle_part_inv_z_scaled_step_left_= Fixed16Div( inv_z_scaled[ more_upper_left  ] - inv_z_scaled[ left_index  ], dy_left  );
			triangle_part_vertices_[0]= vertices[ left_index  ];
			triangle_part_vertices_[1]= vertices[ more_upper_left  ];
			triangle_part_vertices_[2]= vertices[ right_index ];
			triangle_part_vertices_[3]= vertices[ more_upper_right ];
			trianlge_part_tc_left_= tex_coord_div_z[ left_index  ];
			triangle_part_inv_z_scaled_left_= inv_z_scaled[ left_index ];

			// Right steps
			const fixed16_t triangle_part_inv_z_scaled_step_right= Fixed16Div( inv_z_scaled[ more_upper_right ] - inv_z_scaled[ right_index ], dy_right );
			const fixed16_t traingle_part_tc_step_right_u= Fixed16Div( tex_coord_div_z[ more_upper_right ].u - tex_coord_div_z[ right_index ].u, dy_right );
			const fixed16_t traingle_part_tc_step_right_v= Fixed16Div( tex_coord_div_z[ more_upper_right ].v - tex_coord_div_z[ right_index ].v, dy_right );

			const fixed16_t y_min= std::max( triangle_part_vertices_[0].y, triangle_part_vertices_[2].y );
			const fixed16_t y_max= std::min( triangle_part_vertices_[1].y, triangle_part_vertices_[3].y );
			const fixed16_t middle_y= ( y_min + y_max ) / 2;
			const fixed16_t middle_dy_left = middle_y - triangle_part_vertices_[0].y;
			const fixed16_t middle_dy_right= middle_y - triangle_part_vertices_[2].y;

			// Calculate middle line start/end values.
			const fixed16_t x_left = vertices[ left_index  ].x + Fixed16Mul( middle_dy_left , triangle_part_x_step_left_  );
			const fixed16_t x_right= vertices[ right_index ].x + Fixed16Mul( middle_dy_right, triangle_part_x_step_right_ );
			const fixed16_t dx= x_right - x_left;
			if( dx > 0 )
			{
				const fixed16_t inv_z_scaled_left = inv_z_scaled[ left_index  ] + Fixed16Mul( middle_dy_left , triangle_part_inv_z_scaled_step_left_ );
				const fixed16_t inv_z_scaled_right= inv_z_scaled[ right_index ] + Fixed16Mul( middle_dy_right, triangle_part_inv_z_scaled_step_right );
				const fixed16_t tc_left_u = tex_coord_div_z[ left_index  ].u + Fixed16Mul( middle_dy_left , traingle_part_tc_step_left_[0] );
				const fixed16_t tc_left_v = tex_coord_div_z[ left_index  ].v + Fixed16Mul( middle_dy_left , traingle_part_tc_step_left_[1] );
				const fixed16_t tc_right_u= tex_coord_div_z[ right_index ].u + Fixed16Mul( middle_dy_right, traingle_part_tc_step_right_u );
				const fixed16_t tc_right_v= tex_coord_div_z[ right_index ].v + Fixed16Mul( middle_dy_right, traingle_part_tc_step_right_v );

				line_inv_z_scaled_step_= Fixed16Div( inv_z_scaled_right - inv_z_scaled_left, dx );
				line_tc_step_[0]= Fixed16Div( tc_right_u - tc_left_u, dx );
				line_tc_step_[1]= Fixed16Div( tc_right_v - tc_left_v, dx );
				(this->*func)();
			}
		}

		if( vertices[ more_upper_left ].y < vertices[ more_upper_right ].y )
			left_index = more_upper_left ;
		else
			right_index= more_upper_right;
	}
}

template<
	Rasterizer::DepthTest depth_test, Rasterizer::DepthWrite depth_write,
	Rasterizer::AlphaTest alpha_test,
	Rasterizer::OcclusionTest occlusion_test, Rasterizer::OcclusionWrite occlusion_write,
	Rasterizer::Lighting lighting, Rasterizer::Blending blending>
void Rasterizer::DrawAffineTexturedTrianglePart()
{
	const fixed16_t y_start_f= std::max( triangle_part_vertices_[0].y, triangle_part_vertices_[2].y );
	const fixed16_t y_end_f  = std::min( triangle_part_vertices_[1].y, triangle_part_vertices_[3].y );
	const int y_start= std::max( 0, Fixed16RoundToInt( y_start_f ) );
	const int y_end  = std::min( viewport_size_y_, Fixed16RoundToInt( y_end_f ) );

	const fixed16_t y_cut_left = ( y_start << 16 ) + g_fixed16_half - triangle_part_vertices_[0].y;
	const fixed16_t y_cut_right= ( y_start << 16 ) + g_fixed16_half - triangle_part_vertices_[2].y;
	fixed16_t x_left = triangle_part_vertices_[0].x + Fixed16Mul( y_cut_left , triangle_part_x_step_left_  );
	fixed16_t x_right= triangle_part_vertices_[2].x + Fixed16Mul( y_cut_right, triangle_part_x_step_right_ );

	fixed16_t tc_left[2], inv_z_scaled_left;
	tc_left[0]= trianlge_part_tc_left_.u + Fixed16Mul( y_cut_left, traingle_part_tc_step_left_[0] );
	tc_left[1]= trianlge_part_tc_left_.v + Fixed16Mul( y_cut_left, traingle_part_tc_step_left_[1] );
	inv_z_scaled_left= triangle_part_inv_z_scaled_left_ + Fixed16Mul( y_cut_left, triangle_part_inv_z_scaled_step_left_ );

	for(
		int y= y_start;
		y< y_end;
		y++,
		x_left += triangle_part_x_step_left_ ,
		x_right+= triangle_part_x_step_right_,
		tc_left[0]+= traingle_part_tc_step_left_[0],
		tc_left[1]+= traingle_part_tc_step_left_[1],
		inv_z_scaled_left+= triangle_part_inv_z_scaled_step_left_ )
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

		const fixed16_t x_cut= ( x_start << 16 ) + g_fixed16_half - x_left;

		fixed16_t line_tc[2], line_tc_step[2];
		line_tc[0]= tc_left[0] + Fixed16Mul( x_cut, line_tc_step_[0] );
		line_tc[1]= tc_left[1] + Fixed16Mul( x_cut, line_tc_step_[1] );
		line_tc_step[0]= line_tc_step_[0];
		line_tc_step[1]= line_tc_step_[1];

		// Correct texture coordinates. We must NOT read using texture coordinates outside texture.
		if( line_tc[0] < 0 ) line_tc[0]= 0;
		else if( line_tc[0] > max_valid_tc_u_ ) line_tc[0]= max_valid_tc_u_;
		if( line_tc[1] < 0 ) line_tc[1]= 0;
		else if( line_tc[1] > max_valid_tc_v_ ) line_tc[1]= max_valid_tc_v_;

		const int dx= x_end - x_start - 1;
		if( dx > 0 )
		{
			if( line_tc[0] + dx * line_tc_step[0] < 0 )
				line_tc_step[0]= ( -line_tc[0] ) / dx;
			else if( line_tc[0] + dx * line_tc_step[0] > max_valid_tc_u_ )
				line_tc_step[0]= ( max_valid_tc_u_ - line_tc[0] ) / dx;
			if( line_tc[1] + dx * line_tc_step[1] < 0 )
				line_tc_step[1]= ( -line_tc[1] ) / dx;
			else if( line_tc[1] + dx * line_tc_step[1] > max_valid_tc_v_ )
				line_tc_step[1]= ( max_valid_tc_v_ - line_tc[1] ) / dx;
		}

		fixed_base_t line_inv_z_scaled= inv_z_scaled_left + Fixed16Mul( x_cut, line_inv_z_scaled_step_ );

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
				const int u= line_tc[0] >> 16;
				const int v= line_tc[1] >> 16;
				PC_ASSERT( u >= 0 && u < texture_size_x_ );
				PC_ASSERT( v >= 0 && v < texture_size_y_ );
				const uint32_t tex_value= texture_data_[ u + v * texture_size_x_ ];

				if( alpha_test == AlphaTest::Yes && (tex_value & c_alpha_mask) == 0u )
					continue;

				if( depth_write == DepthWrite::Yes ) depth_dst[x]= depth;
				if( occlusion_write == OcclusionWrite::Yes ) occlusion_dst[ x >> 3u ] |= 1u << (x&7u); // TODO - maybe set occlusion at end of line processing?

				ApplyBlending<blending>( dst[x], ApplyLight<lighting>( tex_value ) );
			}
		}
	} // for y
}

template<
	Rasterizer::DepthTest depth_test, Rasterizer::DepthWrite depth_write,
	Rasterizer::AlphaTest alpha_test,
	Rasterizer::OcclusionTest occlusion_test, Rasterizer::OcclusionWrite occlusion_write,
	Rasterizer::Lighting lighting, Rasterizer::Blending blending>
void Rasterizer::DrawTexturedTrianglePerLineCorrectedPart()
{
	const fixed16_t y_start_f= std::max( triangle_part_vertices_[0].y, triangle_part_vertices_[2].y );
	const fixed16_t y_end_f  = std::min( triangle_part_vertices_[1].y, triangle_part_vertices_[3].y );
	const int y_start= std::max( 0, Fixed16RoundToInt( y_start_f ) );
	const int y_end  = std::min( viewport_size_y_, Fixed16RoundToInt( y_end_f ) );

	const fixed16_t y_cut_left = ( y_start << 16 ) + g_fixed16_half - triangle_part_vertices_[0].y;
	const fixed16_t y_cut_right= ( y_start << 16 ) + g_fixed16_half - triangle_part_vertices_[2].y;
	fixed16_t x_left = triangle_part_vertices_[0].x + Fixed16Mul( y_cut_left , triangle_part_x_step_left_  );
	fixed16_t x_right= triangle_part_vertices_[2].x + Fixed16Mul( y_cut_right, triangle_part_x_step_right_ );

	fixed16_t tc_div_z_left[2], inv_z_scaled_left;
	tc_div_z_left[0]= trianlge_part_tc_left_.u + Fixed16Mul( y_cut_left, traingle_part_tc_step_left_[0] );
	tc_div_z_left[1]= trianlge_part_tc_left_.v + Fixed16Mul( y_cut_left, traingle_part_tc_step_left_[1] );
	inv_z_scaled_left= triangle_part_inv_z_scaled_left_ + Fixed16Mul( y_cut_left, triangle_part_inv_z_scaled_step_left_ );

	for(
		int y= y_start;
		y< y_end;
		y++,
		x_left += triangle_part_x_step_left_ ,
		x_right+= triangle_part_x_step_right_,
		tc_div_z_left[0]+= traingle_part_tc_step_left_[0],
		tc_div_z_left[1]+= traingle_part_tc_step_left_[1],
		inv_z_scaled_left+= triangle_part_inv_z_scaled_step_left_ )
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
				const int u= line_tc[0] >> 16;
				const int v= line_tc[1] >> 16;
				PC_ASSERT( u >= 0 && u < texture_size_x_ );
				PC_ASSERT( v >= 0 && v < texture_size_y_ );
				const uint32_t tex_value= texture_data_[ u + v * texture_size_x_ ];

				if( alpha_test == AlphaTest::Yes && (tex_value & c_alpha_mask) == 0u )
					continue;

				if( depth_write == DepthWrite::Yes ) depth_dst[x]= depth;
				if( occlusion_write == OcclusionWrite::Yes ) occlusion_dst[ x >> 3u ] |= 1u << (x&7u); // TODO - maybe set occlusion at end of line processing?

				ApplyBlending<blending>( dst[x], ApplyLight<lighting>( tex_value ) );
			}
		}
	} // for y
}

template<
	Rasterizer::DepthTest depth_test, Rasterizer::DepthWrite depth_write,
	Rasterizer::AlphaTest alpha_test,
	Rasterizer::OcclusionTest occlusion_test, Rasterizer::OcclusionWrite occlusion_write,
	Rasterizer::Lighting lighting, Rasterizer::Blending blending>
void Rasterizer::DrawTexturedTriangleSpanCorrectedPart()
{
	// TODO - maybe add mmx lighting support for other triangle-filling functions?
#ifdef PC_MMX_INSTRUCTIONS
	__m64 mm_light; // Store light in 10.6 fixed format.
	__m64 mm_zero;
	constexpr int c_mm_light_shift= 2;
	if( lighting == Lighting::Yes )
	{
		mm_zero= _mm_setzero_si64();
		mm_light= _mm_set1_pi16( light_ >> c_mm_light_shift );
	}

	#define DO_LIGHTING(tex_value, destination)\
	if( lighting == Lighting::Yes )\
	{\
		__m64 components= _mm_cvtsi32_si64( tex_value ); /* write 4 byte componenst to register */\
		__m64 components_depacked= _mm_unpacklo_pi8( components, mm_zero ); /* convert* 8bit components to 16bit components */\
		__m64 mul_result= _mm_mulhi_pi16( mm_light, components_depacked ); /* multiplicate light by each color component, save hight bits */\
		__m64 result_shifted= _m_psllwi( mul_result, c_mm_light_shift ); /* compensate light shift */\
		__m64 result_8bit= _m_packuswb( result_shifted, mm_zero ); /* pack 16-bit colors to 8-bits colors with unsigned saturation */\
		ApplyBlending<blending>( destination, _m_to_int(result_8bit) );\
	}\
	else\
		ApplyBlending<blending>( destination, ApplyLight<lighting>(tex_value) );
#else
	#define DO_LIGHTING(tex_value, destination) destination= ApplyLight<lighting>(tex_value);
#endif

	const fixed16_t y_start_f= std::max( triangle_part_vertices_[0].y, triangle_part_vertices_[2].y );
	const fixed16_t y_end_f= std::min( triangle_part_vertices_[1].y, triangle_part_vertices_[3].y );
	const int y_start= std::max( 0, Fixed16RoundToInt( y_start_f ) );
	const int y_end  = std::min( viewport_size_y_, Fixed16RoundToInt( y_end_f ) );

	const fixed16_t y_cut_left = ( y_start << 16 ) + g_fixed16_half - triangle_part_vertices_[0].y;
	const fixed16_t y_cut_right= ( y_start << 16 ) + g_fixed16_half - triangle_part_vertices_[2].y;
	fixed16_t x_left = triangle_part_vertices_[0].x + Fixed16Mul( y_cut_left , triangle_part_x_step_left_  );
	fixed16_t x_right= triangle_part_vertices_[2].x + Fixed16Mul( y_cut_right, triangle_part_x_step_right_ );

	fixed16_t tc_div_z_left[2], inv_z_scaled_left;
	tc_div_z_left[0]= trianlge_part_tc_left_.u + Fixed16Mul( y_cut_left, traingle_part_tc_step_left_[0] );
	tc_div_z_left[1]= trianlge_part_tc_left_.v + Fixed16Mul( y_cut_left, traingle_part_tc_step_left_[1] );
	inv_z_scaled_left= triangle_part_inv_z_scaled_left_ + Fixed16Mul( y_cut_left, triangle_part_inv_z_scaled_step_left_ );

	for(
		int y= y_start;
		y< y_end;
		y++,
		x_left += triangle_part_x_step_left_ ,
		x_right+= triangle_part_x_step_right_,
		tc_div_z_left[0]+= traingle_part_tc_step_left_[0],
		tc_div_z_left[1]+= traingle_part_tc_step_left_[1],
		inv_z_scaled_left+= triangle_part_inv_z_scaled_step_left_ )
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
					const int u= span_tc[0] >> 16;
					const int v= span_tc[1] >> 16;
					PC_ASSERT( u >= 0 && u < texture_size_x_ );
					PC_ASSERT( v >= 0 && v < texture_size_y_ );
					const uint32_t tex_value= texture_data_[ u + v * texture_size_x_ ];

					if( alpha_test == AlphaTest::Yes && (tex_value & c_alpha_mask) == 0u )
						continue;
					if( depth_write == DepthWrite::Yes ) depth_dst[ full_x ]= depth;
					if( occlusion_write == OcclusionWrite::Yes ) occlusion_dst[ full_x >> 3 ] |= 1 << (full_x&7);  // TODO - maybe set occlusion at end of line processing?

					DO_LIGHTING(tex_value, dst[full_x]);
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
					const int u= span_tc[0] >> 16;
					const int v= span_tc[1] >> 16;
					PC_ASSERT( u >= 0 && u < texture_size_x_ );
					PC_ASSERT( v >= 0 && v < texture_size_y_ );
					const uint32_t tex_value= texture_data_[ u + v * texture_size_x_ ];

					if( alpha_test == AlphaTest::Yes && (tex_value & c_alpha_mask) == 0u )
						continue;
					if( depth_write == DepthWrite::Yes ) depth_dst[ span_x + x ]= depth;
					if( occlusion_write == OcclusionWrite::Yes && alpha_test == AlphaTest::Yes ) occlusion_value|= 1 << x;

					DO_LIGHTING(tex_value, dst[ span_x + x ]);
				}
			} // for span pixels

			// TODO - maybe set occlusion at end of line processing?
			if( occlusion_write == OcclusionWrite::Yes )
			{
				if( alpha_test == AlphaTest::Yes )
					*reinterpret_cast<SpanOcclusionType*>( occlusion_dst + (span_x >> 3) ) = occlusion_value;
				else
					*reinterpret_cast<SpanOcclusionType*>( occlusion_dst + (span_x >> 3) ) = c_span_occlusion_value;
			}

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
					const int u= span_tc[0] >> 16;
					const int v= span_tc[1] >> 16;
					PC_ASSERT( u >= 0 && u < texture_size_x_ );
					PC_ASSERT( v >= 0 && v < texture_size_y_ );
					const uint32_t tex_value= texture_data_[ u + v * texture_size_x_ ];

					if( alpha_test == AlphaTest::Yes && (tex_value & c_alpha_mask) == 0u )
						continue;

					if( depth_write == DepthWrite::Yes ) depth_dst[x]= depth;
					if( occlusion_write == OcclusionWrite::Yes ) occlusion_dst[ x >> 3 ] |= 1 << (x&7); // TODO - maybe set occlusion at end of line processing?

					DO_LIGHTING(tex_value, dst[x]);
				}
			}
		}
	} // for y

#ifdef PC_MMX_INSTRUCTIONS
	// At end of using MMX we need call "emms" instructions for correct usage of FPU after this function.
	if( lighting == Lighting::Yes )
		_mm_empty();
#endif
	#undef DO_LIGHTING // Remove function-local macro.
}

template<
	Rasterizer::DepthTest depth_test, Rasterizer::DepthWrite depth_write,
	Rasterizer::AlphaTest alpha_test,
	Rasterizer::OcclusionTest occlusion_test, Rasterizer::OcclusionWrite occlusion_write,
	Rasterizer::Lighting lighting, Rasterizer::Blending blending>
void Rasterizer::DrawTexturedTrianglePerLineCorrected( const RasterizerVertex* vertices )
{
	Rasterizer::DrawTrianglePerspectiveCorrectedImpl<
		TrianglePartDrawFunc,
		&Rasterizer::DrawTexturedTrianglePerLineCorrectedPart<depth_test, depth_write, alpha_test, occlusion_test, occlusion_write, lighting, blending > >
			( vertices );
}

template<
	Rasterizer::DepthTest depth_test, Rasterizer::DepthWrite depth_write,
	Rasterizer::AlphaTest alpha_test,
	Rasterizer::OcclusionTest occlusion_test, Rasterizer::OcclusionWrite occlusion_write,
	Rasterizer::Lighting lighting, Rasterizer::Blending blending>
void Rasterizer::DrawTexturedTriangleSpanCorrected( const RasterizerVertex* vertices )
{

	DrawTrianglePerspectiveCorrectedImpl<
		TrianglePartDrawFunc,
		&Rasterizer::DrawTexturedTriangleSpanCorrectedPart<depth_test, depth_write, alpha_test, occlusion_test, occlusion_write, lighting, blending > >
			( vertices );
}

template<
	Rasterizer::DepthTest depth_test, Rasterizer::DepthWrite depth_write,
	Rasterizer::AlphaTest alpha_test,
	Rasterizer::OcclusionTest occlusion_test, Rasterizer::OcclusionWrite occlusion_write,
	Rasterizer::Lighting lighting, Rasterizer::Blending blending>
void Rasterizer::DrawTexturedConvexPolygonSpanCorrected(  const RasterizerVertex* vertices, unsigned int vertex_count, bool is_anticlockwise )
{

	DrawConvexPolygonPerspectiveCorrectedImpl<
		TrianglePartDrawFunc,
		&Rasterizer::DrawTexturedTriangleSpanCorrectedPart<depth_test, depth_write, alpha_test, occlusion_test, occlusion_write, lighting, blending > >
			( vertices, vertex_count, is_anticlockwise );
}

} // namespace PanzerChasm
