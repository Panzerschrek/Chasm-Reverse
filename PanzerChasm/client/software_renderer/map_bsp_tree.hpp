#pragma once
#include <plane.hpp>

#include "../../fwd.hpp"

namespace PanzerChasm
{

class MapBSPTree final
{
public:
	struct WallSegment
	{
		unsigned int wall_index;
		float start, end; // [0 .0f - 1.0f ]
	};

	static constexpr unsigned int c_null_node= 0u;

	struct Node
	{
		m_Plane2 plane;

		// Segments on node plane
		unsigned int first_segment;
		unsigned int segment_count;

		// Zero - if has no child.
		unsigned int node_front, node_back;
	};

public:
	explicit MapBSPTree( const MapDataConstPtr& map_data );

private:
	typedef std::vector<unsigned short> WallsSet;

private:
	// Returns new node number.
	unsigned int BuildTree_r( const WallsSet& walls );

private:
	const MapDataConstPtr map_data_;

	std::vector<WallSegment> segments_;

	unsigned int root_node_;
	std::vector<Node> nodes_;
};


} // namespace PanzerChasm
