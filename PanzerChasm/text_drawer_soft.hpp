#pragma once

#include "fwd.hpp"
#include "i_text_drawer.hpp"
#include "rendering_context.hpp"
#include "text_drawers_common.hpp"

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
	const RenderingContextSoft rendering_context_;
	unsigned char letters_width_[256];

	unsigned char font_texture_data_[ FontParams::atlas_width * FontParams::atlas_height * FontParams::colors_variations ];
};

} // namespace PanzerChasm
