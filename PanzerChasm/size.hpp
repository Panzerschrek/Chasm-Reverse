#pragma once

namespace PanzerChasm
{

struct Size2
{
	unsigned int xy[2];

	Size2(){}

	Size2( const unsigned int width, const unsigned int height )
	{
		xy[0]= width; xy[1]= height;
	}

	unsigned int& Width () { return xy[0]; }
	unsigned int& Height() { return xy[1]; }

	unsigned int Width () const { return xy[0]; }
	unsigned int Height() const { return xy[1]; }
};

inline bool operator==( const Size2& l, const Size2& r )
{
	return l.xy[0] == r.xy[0] && l.xy[1] == r.xy[1];
}

inline bool operator!=( const Size2& l, const Size2& r )
{
	return !( l == r );
}

} // namespace PanzerChasm
