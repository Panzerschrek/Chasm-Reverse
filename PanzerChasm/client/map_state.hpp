#pragma once
#include "../game_resources.hpp"
#include "../map_loader.hpp"
#include "../messages.hpp"
#include "../time.hpp"

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
	MapState(
		const MapDataConstPtr& map,
		const GameResourcesConstPtr& game_resources,
		Time map_start_time );
	~MapState();

	const DynamicWalls& GetDynamicWalls() const;
	const StaticModels& GetStaticModels() const;
	const Items& GetItems() const;

	void Tick( Time current_time );

	void ProcessMessage( const Messages::EntityState& message );
	void ProcessMessage( const Messages::WallPosition& message );
	void ProcessMessage( const Messages::StaticModelState& message );
	void ProcessMessage( const Messages::EntityBirth& message );
	void ProcessMessage( const Messages::EntityDeath& message );

private:
	const MapDataConstPtr map_data_;
	const GameResourcesConstPtr game_resources_;
	const Time map_start_time_;

	DynamicWalls dynamic_walls_;
	StaticModels static_models_;
	Items items_;
};

} // namespace PanzerChasm
