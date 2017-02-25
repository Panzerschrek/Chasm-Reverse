#pragma once
#include <vector>

#include <vec.hpp>

#include "../fwd.hpp"
#include "fwd.hpp"

namespace PanzerChasm
{

class MinimapState final
{
public:
	explicit MinimapState( const MapDataConstPtr& map_data );
	~MinimapState();

	void Update(
		const MapState& map_state,
		const m_Vec2& camera_position,
		float view_angle_z );

private:
	const MapDataConstPtr map_data_;

	std::vector<bool> static_walls_visibility_;
	std::vector<bool> dynamic_walls_visibility_;
};

} // namespace PanzerChasm
