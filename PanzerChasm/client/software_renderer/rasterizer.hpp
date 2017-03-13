#pragma once
#include <cstdint>

#include "fixed.hpp"

namespace PanzerChasm
{

struct RasterizerVertexSimple
{
	fixed16_t x, y; // Screen space
};

struct RasterizerTexCoord
{
	fixed16_t u, v;
};

struct RasterizerVertexTextured : public RasterizerVertexSimple, public RasterizerTexCoord
{};

struct RasterizerVertexXYZ : public RasterizerVertexSimple
{
	fixed16_t z;
};

class Rasterizer final
{
public:
	Rasterizer(
		unsigned int viewport_size_x,
		unsigned int viewport_size_y,
		unsigned int row_size /* Greater or equal to viewport_size_x */,
		uint32_t* color_buffer );

	~Rasterizer();

	void SetTexture(
		unsigned int size_x,
		unsigned int size_y,
		const uint32_t* data );

	void DrawAffineColoredTriangle( const RasterizerVertexSimple* trianlge_vertices, uint32_t color );
	void DrawAffineTexturedTriangle( const RasterizerVertexTextured* trianlge_vertices );

private:
	void DrawAffineColoredTrianglePart( uint32_t color );
	void DrawAffineTexturedTrianglePart();

private:
	// Use only SIGNED types inside rasterizer.

	// Buffer info
	const int viewport_size_x_;
	const int viewport_size_y_;
	const int row_size_;
	uint32_t* const color_buffer_;

	// Texture
	int texture_size_x_= 0;
	int texture_size_y_= 0;
	const uint32_t* texture_data_= nullptr;

	// Intermediate variables

	// 0 - lower left
	// 1 - upper left
	// 2 - lower right
	// 3 - upper right
	RasterizerVertexSimple triangle_part_vertices_[4];
	RasterizerTexCoord triangle_part_tex_coords_[4];
};

} // namespace PanzerChasm
