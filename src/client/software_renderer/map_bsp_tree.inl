#pragma once
#include "../../assert.hpp"
#include "map_bsp_tree.hpp"

namespace PanzerChasm
{

template<class Func>
void MapBSPTree::EnumerateSegmentsFrontToBack( const m_Vec2& camera_position, const Func& func ) const
{
	EnumerateSegmentsFrontToBack_r( nodes_[root_node_], camera_position, func );
}

template<class Func>
void MapBSPTree::EnumerateSegmentsFrontToBack_r( const Node& node, const m_Vec2& camera_position, const Func& func ) const
{
	const bool at_front= node.plane.IsPointAheadPlane( camera_position );

	unsigned int node_front, node_back;
	if( at_front )
	{
		node_front= node.node_front;
		 node_back= node. node_back;
	}
	else
	{
		node_front= node. node_back;
		 node_back= node.node_front;
	}

	if( node_front != c_null_node )
	{
		PC_ASSERT( node_front < nodes_.size() );
		EnumerateSegmentsFrontToBack_r( nodes_[ node_front ], camera_position, func );
	}

	for( unsigned int segment= 0u; segment < node.segment_count; segment++ )
	{
		unsigned int segment_number= node.first_segment + segment;
		PC_ASSERT( segment_number < segments_.size() );
		func( segments_[ segment_number ] );
	}

	if(  node_back != c_null_node )
	{
		PC_ASSERT(  node_back < nodes_.size() );
		EnumerateSegmentsFrontToBack_r( nodes_[  node_back ], camera_position, func );
	}
}

} // namespace PanzerChasm
