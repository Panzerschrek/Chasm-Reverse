#pragma once
#include <cstdint>
#include <vector>

#include "fixed.hpp"

namespace PanzerChasm
{

struct RasterizerVertexCoord
{
	fixed16_t x, y; // Screen space
};

struct RasterizerVertexTexCoord
{
	fixed16_t u, v;
};

struct RasterizerVertex : public RasterizerVertexCoord, public RasterizerVertexTexCoord
{
	fixed16_t z;
};

class Rasterizer final
{
public:
	static constexpr int c_max_inv_z_min_log2= 4; // z_min = 1.0f / 16.0f
	static constexpr int c_inv_z_scaler_log2= 11;
	static constexpr int c_inv_z_scaler= 1 << c_inv_z_scaler_log2;

	// 16 pixels - should be enough. 8 pixels - overkill, 32 pixels - too inaccurate.
	static constexpr int c_z_correct_span_size_log2= 4;
	static constexpr int c_z_correct_span_size= 1 << c_z_correct_span_size_log2;
	static constexpr int c_z_correct_span_size_minus_one= c_z_correct_span_size - 1;

	typedef void (Rasterizer::*TriangleDrawFunc)(const RasterizerVertex*);

	Rasterizer(
		unsigned int viewport_size_x,
		unsigned int viewport_size_y,
		unsigned int row_size /* Greater or equal to viewport_size_x */,
		uint32_t* color_buffer );

	~Rasterizer();

	void ClearDepthBuffer();
	void BuildDepthBufferHierarchy();

	bool IsDepthOccluded(
		fixed16_t x_min, fixed16_t y_min, fixed16_t x_max, fixed16_t y_max,
		fixed16_t z_min, fixed16_t z_max ) const;

	void DebugDrawDepthHierarchy( unsigned int tick_count );

	void SetTexture(
		unsigned int size_x,
		unsigned int size_y,
		const uint32_t* data );

	void DrawAffineColoredTriangle( const RasterizerVertex* trianlge_vertices, uint32_t color );
	void DrawAffineTexturedTriangle( const RasterizerVertex* trianlge_vertices );
	void DrawTexturedTrianglePerLineCorrected( const RasterizerVertex* trianlge_vertices );
	void DrawTexturedTriangleSpanCorrected( const RasterizerVertex* trianlge_vertices );

private:
	template< class TrianglePartDrawFunc, TrianglePartDrawFunc func>
	void DrawTrianglePerspectiveCorrectedImpl( const RasterizerVertex* trianlge_vertices );

	void DrawAffineColoredTrianglePart( uint32_t color );
	void DrawAffineTexturedTrianglePart();
	void DrawTexturedTrianglePerLineCorrectedPart();
	void DrawTexturedTriangleSpanCorrectedPart();

private:
	// Use only SIGNED types inside rasterizer.

	// Buffer info
	const int viewport_size_x_;
	const int viewport_size_y_;
	const int row_size_;
	uint32_t* const color_buffer_;

	// Depth buffer
	std::vector<unsigned short> depth_buffer_storage_;
	unsigned short* depth_buffer_;
	int depth_buffer_width_;

	// Depth buffer hierarchy
	static constexpr unsigned int c_depth_buffer_hierarchy_levels= 6u;
	static constexpr unsigned int c_first_depth_hierarchy_level_size= 4u;
	// First level - 4x4
	//   4x  4
	//   8x  8
	//  16x 16
	//  32x 32
	//  64x 64
	// 128x128
	struct
	{
		unsigned short* data;
		unsigned int width, height;
	} depth_buffer_hierarchy_[ c_depth_buffer_hierarchy_levels ];

	// Texture
	int texture_size_x_= 0;
	int texture_size_y_= 0;
	fixed16_t max_valid_tc_u_= 0;
	fixed16_t max_valid_tc_v_= 0;
	const uint32_t* texture_data_= nullptr;

	// Intermediate variables

	// 0 - lower left
	// 1 - upper left
	// 2 - lower right
	// 3 - upper right
	RasterizerVertexCoord triangle_part_vertices_[4];
	// 0 - lower left   1 - upper left
	RasterizerVertexTexCoord triangle_part_tex_coords_[2]; // For perspective-corrected methods = tc / z
	fixed_base_t triangle_part_inv_z_scaled[2];

	// For perspective-corrected methods = tc / z
	fixed16_t line_tc_step_[2];
	fixed_base_t line_inv_z_scaled_step_;
};

} // namespace PanzerChasm
