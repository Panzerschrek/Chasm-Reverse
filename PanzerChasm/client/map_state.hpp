#pragma once
#include "../map_loader.hpp"
#include "../messages.hpp"

namespace PanzerChasm
{

// Class for accumulating of map state, recieved from server.
class MapState final
{
public:
	struct DynamicWall
	{
		// coordinates - 8.8 format
		short xy[2][2];
		short z; // wall bottom z
	};

	typedef std::vector<DynamicWall> DynamicWalls;

	explicit MapState( const MapDataConstPtr& map );
	~MapState();

	const DynamicWalls& GetDynamicWalls() const;

	void ProcessMessage( const Messages::EntityState& message );
	void ProcessMessage( const Messages::WallPosition& message );
	void ProcessMessage( const Messages::EntityBirth& message );
	void ProcessMessage( const Messages::EntityDeath& message );

private:
	const MapDataConstPtr map_data_;

	DynamicWalls dynamic_walls_;
};

} // namespace PanzerChasm
