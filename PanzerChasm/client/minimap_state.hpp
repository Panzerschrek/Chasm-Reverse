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
	typedef std::vector<bool> WallsVisibility;

	explicit MinimapState( const MapDataConstPtr& map_data );
	~MinimapState();

	void SetState(
		const WallsVisibility& static_walls_visibility ,
		const WallsVisibility& dynamic_walls_visibility );

	void Update(
		const MapState& map_state,
		const m_Vec2& camera_position,
		float view_angle_z );

	const WallsVisibility& GetStaticWallsVisibility() const;
	const WallsVisibility& GetDynamicWallsVisibility() const;

private:
	const MapDataConstPtr map_data_;

	WallsVisibility static_walls_visibility_;
	WallsVisibility dynamic_walls_visibility_;
};

} // namespace PanzerChasm
