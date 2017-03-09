#pragma once
#include "i_drawers_factory.hpp"
#include "rendering_context.hpp"

namespace PanzerChasm
{

class DrawersFactorySoft final : public IDrawersFactory
{
public:
	DrawersFactorySoft(
		Settings& settings,
		const GameResourcesConstPtr& game_resources,
		const RenderingContextSoft& rendering_context );

	virtual ~DrawersFactorySoft() override;

	virtual ITextDrawerPtr CreateTextDrawer() override;
	virtual IMenuDrawerPtr CreateMenuDrawer() override;
	virtual IHudDrawerPtr CreateHUDDrawer( const SharedDrawersPtr& shared_drawers ) override;
	virtual IMapDrawerPtr CreateMapDrawer() override;
	virtual IMinimapDrawerPtr CreateMinimapDrawer() override;

private:
	Settings& settings_;
	const GameResourcesConstPtr game_resources_;
	const RenderingContextSoft rendering_context_;
};

} // namespace PanzerChasm
