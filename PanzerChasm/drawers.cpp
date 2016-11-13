#include "drawers.hpp"

namespace PanzerChasm
{

Drawers::Drawers(
	const RenderingContext& rendering_context,
	const GameResources& game_resources )
	: menu( rendering_context, game_resources )
	, text( rendering_context, game_resources )
{}

Drawers::~Drawers()
{}

} // namespace PanzerChasm
