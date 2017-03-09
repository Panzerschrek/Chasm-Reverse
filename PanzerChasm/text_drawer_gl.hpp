#pragma once

#include <glsl_program.hpp>
#include <polygon_buffer.hpp>
#include <texture.hpp>

#include "fwd.hpp"
#include "i_text_drawer.hpp"
#include "rendering_context.hpp"

namespace PanzerChasm
{

class TextDrawerGL final : public ITextDrawer
{
public:
	TextDrawerGL(
		const RenderingContextGL& rendering_context,
		const GameResources& game_resources );
	virtual ~TextDrawerGL() override;

	virtual unsigned int GetLineHeight() const override;

	virtual void Print(
		int x, int y,
		const char* text,
		unsigned int scale,
		FontColor color= FontColor::White,
		Alignment alignment= Alignment::Left ) override;

private:
	struct Vertex
	{
		short xy[2];
		short tex_coord[2];
	};

private:
	const Size2 viewport_size_;
	unsigned char letters_width_[256];

	r_GLSLProgram shader_;
	r_Texture texture_;

	r_PolygonBuffer polygon_buffer_;
	std::vector<Vertex> vertex_buffer_;

};

} // namespace PanzerChasm
