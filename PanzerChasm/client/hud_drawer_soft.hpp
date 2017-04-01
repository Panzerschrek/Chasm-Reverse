#pragma once

#include "../rendering_context.hpp"
#include "hud_drawer_base.hpp"

namespace PanzerChasm
{

class HudDrawerSoft final : public HudDrawerBase
{
public:
	HudDrawerSoft(
		const GameResourcesConstPtr& game_resources,
		const RenderingContextSoft& rendering_context,
		const SharedDrawersPtr& shared_drawers );
	virtual ~HudDrawerSoft() override;

	virtual void DrawCrosshair() override;
	virtual void DrawHud( bool draw_second_hud, const char* map_name ) override;

private:
	struct Image
	{
		unsigned int size[2];
		std::vector<unsigned char> data; // color-indexed.
	};

private:
	void LoadImage( const char* file_name, Image& out_image );

private:
	const RenderingContextSoft& rendering_context_;

	Image crosshair_image_;
	Image weapon_icons_image_;
	Image hud_numbers_image_;
	Image hud_background_image_;
};

} // namespace PanzerChasm
