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

	struct StaticModel
	{
		m_Vec3 pos;
		float angle;
		unsigned int model_id;
		unsigned int animation_frame;
	};

	typedef std::vector<StaticModel> StaticModels;

	struct Item
	{
		m_Vec3 pos;
		float angle;
		unsigned char item_id;
		bool picked_up;
		unsigned int animation_frame;
	};

	typedef std::vector<Item> Items;

public:
	explicit MapState( const MapDataConstPtr& map );
	~MapState();

	const DynamicWalls& GetDynamicWalls() const;
	const StaticModels& GetStaticModels() const;
	const Items& GetItems() const;

	void ProcessMessage( const Messages::EntityState& message );
	void ProcessMessage( const Messages::WallPosition& message );
	void ProcessMessage( const Messages::StaticModelState& message );
	void ProcessMessage( const Messages::EntityBirth& message );
	void ProcessMessage( const Messages::EntityDeath& message );

private:
	const MapDataConstPtr map_data_;

	DynamicWalls dynamic_walls_;
	StaticModels static_models_;
	Items items_;
};

} // namespace PanzerChasm
