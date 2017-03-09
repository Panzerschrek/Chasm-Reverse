#include "assert.hpp"

#include "text_drawer_soft.hpp"

namespace PanzerChasm
{

TextDrawerSoft::TextDrawerSoft(
	const RenderingContextSoft& rendering_context,
	const GameResources& game_resources )
{
	// TODO
	PC_UNUSED(rendering_context);
	PC_UNUSED(game_resources);
}

TextDrawerSoft::~TextDrawerSoft()
{}

unsigned int TextDrawerSoft::GetLineHeight() const
{
	// TODO
	return 0u;
}

void TextDrawerSoft::Print(
	const int x, const int y,
	const char* const  text,
	const unsigned int scale,
	const FontColor color,
	const Alignment alignment )
{
	// TODO
	PC_UNUSED(x);
	PC_UNUSED(y);
	PC_UNUSED(text);
	PC_UNUSED(scale);
	PC_UNUSED(color);
	PC_UNUSED(alignment);
}


} // namespace PanzerChasm
