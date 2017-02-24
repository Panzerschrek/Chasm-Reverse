#pragma once

#include <vec.hpp>

#include "../map_loader.hpp"

namespace PanzerChasm
{

class MovementRestriction final
{
public:
	MovementRestriction();
	~MovementRestriction();

	void AddRestriction( const m_Vec2& normal, const MapData::IndexElement& map_element );

	bool GetRestrictionNormal( m_Vec2& out_optional_normal ) const;

private:
	struct RestrictionPlane
	{
		m_Vec2 normal;
		MapData::IndexElement map_element;
	};

private:
	RestrictionPlane restriction_planes_[2];
	unsigned int planes_count_= 0u;
};

} // namespace PanzerChasm
