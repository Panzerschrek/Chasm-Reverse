#pragma once
#include "size.hpp"

namespace PanzerChasm
{

class IMenuDrawer
{
public:
	enum class MenuPicture
	{
		Main,
		New,
		Network,

		PicturesCount
	};

	enum class PictureColor
	{
		Unactive,
		Active,
		Disabled
	};

public:
	virtual ~IMenuDrawer(){}

	virtual Size2 GetViewportSize() const= 0;
	virtual unsigned int GetMenuScale() const= 0;
	virtual unsigned int GetConsoleScale() const= 0;

	virtual Size2 GetPictureSize( MenuPicture picture ) const= 0;

	virtual void DrawMenuBackground(
		int x, int y,
		unsigned int width, unsigned int height )= 0;

	virtual void DrawMenuPicture(
		int x, int y,
		MenuPicture picture,
		const PictureColor* rows_colors )= 0;

	virtual void DrawConsoleBackground( float console_pos )= 0;
	virtual void DrawLoading( float progress )= 0;
	virtual void DrawPaused()= 0;
	virtual void DrawGameBackground()= 0;
	virtual void DrawBriefBar()= 0;
};

} // namespace PanzerChasm
