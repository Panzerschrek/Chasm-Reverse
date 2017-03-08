#pragma once
#include "i_drawers_factory.hpp"
#include "rendering_context.hpp"

namespace PanzerChasm
{

class DrawersFactoryGL final : public IDrawersFactory
{
public:
	DrawersFactoryGL(
		Settings& settings,
		const GameResourcesConstPtr& game_resources,
		const RenderingContext& rendering_context );

	virtual ~DrawersFactoryGL() override;

	virtual ITextDrawerPtr CreateTextDrawer() override;
	virtual IMenuDrawerPtr CreateMenuDrawer() override;
	virtual IHudDrawerPtr CreateHUDDrawer( const SharedDrawersPtr& shared_drawers ) override;
	virtual IMapDrawerPtr CreateMapDrawer() override;
	virtual IMinimapDrawerPtr CreateMinimapDrawer() override;

private:
	Settings& settings_;
	const GameResourcesConstPtr game_resources_;
	const RenderingContext rendering_context_;
};

} // namespace PanzerChasm
