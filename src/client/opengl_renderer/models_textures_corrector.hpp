#pragma once

#include "../../model.hpp"

namespace PanzerChasm
{

class ModelsTexturesCorrector final
{
public:
	ModelsTexturesCorrector();
	~ModelsTexturesCorrector();

	void CorrectModelTexture(
		const Model& model,
		unsigned char* texture_data_rgba );

private:
	void DrawTriangle( const m_Vec2* vertices, bool alpha_test );
	void DrawTrianglePart( bool alpha_test );

private:
	std::vector<unsigned char> field_;

	int texture_size_[2];
	const unsigned char* texture_data_rgba_;
	m_Vec2 triangle_part_vertices_[4];
	float triangle_part_x_step_left_, triangle_part_x_step_right_;
};

} // namespace PanzerChasm
