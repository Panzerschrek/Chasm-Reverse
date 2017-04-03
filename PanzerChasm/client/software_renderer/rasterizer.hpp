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
	static constexpr uint32_t c_alpha_mask= 0xFF000000u; // TODO - maybe this is not constant?

	static constexpr int c_max_inv_z_min_log2= 4; // z_min = 1.0f / 16.0f
	static constexpr int c_inv_z_scaler_log2= 11;
	static constexpr int c_inv_z_scaler= 1 << c_inv_z_scaler_log2;

	// 16 pixels - should be enough. 8 pixels - overkill, 32 pixels - too inaccurate.
	static constexpr int c_z_correct_span_size_log2= 4;
	static constexpr int c_z_correct_span_size= 1 << c_z_correct_span_size_log2;
	static constexpr int c_z_correct_span_size_minus_one= c_z_correct_span_size - 1;

	typedef unsigned short SpanOcclusionType;
	static_assert(
		std::numeric_limits<SpanOcclusionType>::digits == c_z_correct_span_size,
		"Size of this type must be euqla to span size" );
	static constexpr SpanOcclusionType c_span_occlusion_value= std::numeric_limits<SpanOcclusionType>::max();

	typedef void (Rasterizer::*TriangleDrawFunc)(const RasterizerVertex*);

	enum class DepthTest
	{ Yes, No };
	enum class DepthWrite
	{ Yes, No };
	enum class AlphaTest
	{ Yes, No };
	enum class OcclusionTest
	{ Yes, No };
	enum class OcclusionWrite
	{ Yes, No };
	enum class Lighting
	{ Yes, No };

	Rasterizer(
		unsigned int viewport_size_x,
		unsigned int viewport_size_y,
		unsigned int row_size /* Greater or equal to viewport_size_x */,
		uint32_t* color_buffer );

	~Rasterizer();

	void ClearDepthBuffer();
	void ClearOcclusionBuffer();
	void BuildDepthBufferHierarchy();

	bool IsDepthOccluded(
		fixed16_t x_min, fixed16_t y_min, fixed16_t x_max, fixed16_t y_max,
		fixed16_t z_min, fixed16_t z_max ) const;

	void UpdateOcclusionHierarchy( const RasterizerVertex* polygon_vertices, unsigned int polygon_vertex_count, bool has_alpha );
	bool IsOccluded( const RasterizerVertex* polygon_vertices, unsigned int polygon_vertex_count ) const;

	void DebugDrawDepthHierarchy( unsigned int tick_count );
	void DebugDrawOcclusionBuffer( unsigned int tick_count );

	void SetTexture(
		unsigned int size_x,
		unsigned int size_y,
		const uint32_t* data );

	// final_color= ( light * color ) >> 16
	void SetLight( fixed16_t light );

	void DrawAffineColoredTriangle( const RasterizerVertex* trianlge_vertices, uint32_t color );

	template<
		DepthTest depth_test, DepthWrite depth_write,
		AlphaTest alpha_test,
		OcclusionTest occlusion_test, OcclusionWrite occlusion_write,
		Lighting lighting= Lighting::No>
	void DrawAffineTexturedTriangle( const RasterizerVertex* trianlge_vertices );

	template<
		DepthTest depth_test, DepthWrite depth_write,
		AlphaTest alpha_test,
		OcclusionTest occlusion_test, OcclusionWrite occlusion_write,
		Lighting lighting= Lighting::No>
	void DrawTexturedTrianglePerLineCorrected( const RasterizerVertex* trianlge_vertices );

	template<
		DepthTest depth_test, DepthWrite depth_write,
		AlphaTest alpha_test,
		OcclusionTest occlusion_test, OcclusionWrite occlusion_write,
		Lighting lighting= Lighting::No>
	void DrawTexturedTriangleSpanCorrected( const RasterizerVertex* trianlge_vertices );

private:
	typedef void (Rasterizer::*TrianglePartDrawFunc)();

private:
	// Returns 1, if cell fully occluded, else - 0
	template<unsigned int level>
	unsigned int UpdateOcclusionHierarchyCell_r( unsigned int cell_x, unsigned int cell_y );

	template<unsigned int level>
	void SetToOneOcclusionHierarchyCell_r( unsigned int cell_x, unsigned int cell_y );

	template<Lighting lighting>
	uint32_t ApplyLight( uint32_t texel ) const;

	template< class TrianglePartDrawFunc, TrianglePartDrawFunc func>
	void DrawTrianglePerspectiveCorrectedImpl( const RasterizerVertex* trianlge_vertices );

	void DrawAffineColoredTrianglePart( uint32_t color );

	template<
		DepthTest depth_test, DepthWrite depth_write,
		AlphaTest alpha_test,
		OcclusionTest occlusion_test, OcclusionWrite occlusion_write,
		Lighting lighting>
	void DrawAffineTexturedTrianglePart();

	template<
		DepthTest depth_test, DepthWrite depth_write,
		AlphaTest alpha_test,
		OcclusionTest occlusion_test, OcclusionWrite occlusion_write,
		Lighting lighting>
	void DrawTexturedTrianglePerLineCorrectedPart();

	template<
		DepthTest depth_test, DepthWrite depth_write,
		AlphaTest alpha_test,
		OcclusionTest occlusion_test, OcclusionWrite occlusion_write,
		Lighting lighting>
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

	// Occlusion buffer.
	// Each bit in buffer is attribute of pixel. 0 - means unfilled.
	std::vector<uint8_t> occlusion_buffer_storage_;
	uint8_t* occlusion_buffer_;
	int occlusion_buffer_width_; // in bytes
	int occlusion_buffer_height_; // in pixels

	// Occlusion buffer hierarchy.
	//   4x  4 bits
	//  16x 16 bits
	//  64x 64 bits
	static constexpr unsigned int c_occlusion_hierarchy_levels= 3u;
	struct
	{
		// Each 16-bit word is bits of lower hierarchy 4x4 block.
		// bit_number= x + y * 4
		unsigned short* data;

		unsigned int size[2];

	} occlusion_hierarchy_levels_[ c_occlusion_hierarchy_levels ];
	std::vector<unsigned short> occlusion_heirarchy_storage_;

	// Texture
	int texture_size_x_= 0;
	int texture_size_y_= 0;
	fixed16_t max_valid_tc_u_= 0;
	fixed16_t max_valid_tc_v_= 0;
	const uint32_t* texture_data_= nullptr;

	// Light
	fixed16_t light_= g_fixed16_one;

	// Intermediate variables

	// 0 - lower left
	// 1 - upper left
	// 2 - lower right
	// 3 - upper right
	RasterizerVertexCoord triangle_part_vertices_[4];
	RasterizerVertexTexCoord trianlge_part_tc_left_;
	fixed_base_t triangle_part_inv_z_scaled_left_;
	fixed16_t triangle_part_x_step_left_, triangle_part_x_step_right_;
	fixed16_t traingle_part_tc_step_left_[2]; // For perspective-corrected methods = tc / z
	fixed16_t triangle_part_inv_z_scaled_step_left_;

	// 0 - lower left   1 - upper left
	//RasterizerVertexTexCoord triangle_part_tex_coords_[2];
	//fixed_base_t triangle_part_inv_z_scaled[2];

	// For perspective-corrected methods = tc / z
	fixed16_t line_tc_step_[2];
	fixed_base_t line_inv_z_scaled_step_;
};

} // namespace PanzerChasm
