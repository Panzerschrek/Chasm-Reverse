#pragma once

#include <glsl_program.hpp>
#include <polygon_buffer.hpp>
#include <texture.hpp>

#include "game_resources.hpp"

namespace PanzerChasm
{

class TextDraw final
{
public:
	enum class FontColor
	{
		White= 0u,
		DrakYellow,
		Golden,
		YellowGreen,
	};

	TextDraw(
		unsigned int viewport_width, unsigned int viewport_height,
		const GameResources& game_resources );
	~TextDraw();

	unsigned int GetLineWidth() const;

	void Print( int x, int y, const char* text, unsigned int scale, FontColor color= FontColor::White );

private:
	struct Vertex
	{
		short xy[2];
		short tex_coord[2];
	};

private:
	unsigned int viewport_size_[2];
	unsigned char letters_width_[256];

	r_GLSLProgram shader_;
	r_Texture texture_;

	r_PolygonBuffer polygon_buffer_;
	std::vector<Vertex> vertex_buffer_;

};

} // namespace PanzerChasm
