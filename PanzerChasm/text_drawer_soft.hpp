#pragma once

#include "fwd.hpp"
#include "i_text_drawer.hpp"
#include "rendering_context.hpp"

namespace PanzerChasm
{

class TextDrawerSoft final : public ITextDrawer
{
public:
	TextDrawerSoft(
		const RenderingContextSoft& rendering_context,
		const GameResources& game_resources );
	virtual ~TextDrawerSoft() override;

	virtual unsigned int GetLineHeight() const override;

	virtual void Print(
		int x, int y,
		const char* text,
		unsigned int scale,
		FontColor color,
		Alignment alignment ) override;

private:

};

} // namespace PanzerChasm
