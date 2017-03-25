#include "../../assert.hpp"
#include "../../map_loader.hpp"

#include "map_bsp_tree.hpp"

namespace PanzerChasm
{

MapBSPTree::MapBSPTree( const MapDataConstPtr& map_data )
	: map_data_(map_data)
{
	PC_ASSERT( map_data_ != nullptr );

	nodes_.emplace_back(); // Dummy node

	WallsSet walls_set;
	walls_set.resize( map_data_->static_walls.size() );
	for( unsigned int i= 0u; i < walls_set.size(); i++ )
		walls_set[i]= i;

	root_node_= BuildTree_r( walls_set );
}

unsigned int MapBSPTree::BuildTree_r( const WallsSet& walls )
{
	// constexpr unsigned int c_min_walls_for_check= 16u;
	const float c_plane_dist_eps= 1.0f / 256.0f;

	int best_score= std::numeric_limits<int>::max();
	WallsSet::value_type best_splitter_wall_number= map_data_->static_walls.size();

	// Select best splitter.
	// TODO - if we have too much walls - check not all walls, but some percent of random walls.
	for( const WallsSet::value_type& splitter_wall_number : walls )
	{
		PC_ASSERT( splitter_wall_number < map_data_->static_walls.size() );
		const MapData::Wall& splitter_wall= map_data_->static_walls[ splitter_wall_number ];

		m_Plane2 plane;
		plane.normal.x= splitter_wall.vert_pos[1].y - splitter_wall.vert_pos[0].y;
		plane.normal.y= splitter_wall.vert_pos[0].x - splitter_wall.vert_pos[1].x;
		const float wall_square_length= plane.normal.SquareLength();
		if( wall_square_length <= 0.0f )
			continue;

		plane.normal/= std::sqrt( wall_square_length );
		plane.dist= -( splitter_wall.vert_pos[0] * plane.normal );

		int on_plane_count= 0;
		int splitted_count= 0;
		int front_count= 0;
		int back_count= 0;
		for( const WallsSet::value_type& wall_number : walls )
		{
			PC_ASSERT( wall_number < map_data_->static_walls.size() );
			const MapData::Wall& wall= map_data_->static_walls[ wall_number ];

			// Force mark splitter as wall on plane. Reduce calculation errors.
			if( wall_number == splitter_wall_number )
			{
				on_plane_count++;
				continue;
			}

			float dist0= plane.GetSignedDistance( wall.vert_pos[0] );
			float dist1= plane.GetSignedDistance( wall.vert_pos[1] );
			if( dist0 > c_plane_dist_eps && dist1 > c_plane_dist_eps )
				front_count++;
			else if( dist0 < -c_plane_dist_eps && dist1 < -c_plane_dist_eps )
				back_count++;
			else if( std::abs(dist0) <= c_plane_dist_eps && std::abs(dist1) <= c_plane_dist_eps )
				on_plane_count++;
			else
				splitted_count++;
		}

		// TODO - calibrate score formula. Maybe, substract some score for walls along X or Y axis?
		// score - smaller means better.
		int score= std::abs( front_count - back_count ) - 2 * on_plane_count + 2 * splitted_count;
		if( score < best_score )
		{
			best_score= score;
			best_splitter_wall_number= splitter_wall_number;
		}
	} // fo splitter candidates.

	PC_ASSERT( best_splitter_wall_number < map_data_->static_walls.size() );
	const MapData::Wall& splitter_wall= map_data_->static_walls[ best_splitter_wall_number ];

	m_Plane2 splitter_plane;
	splitter_plane.normal.x= splitter_wall.vert_pos[1].y - splitter_wall.vert_pos[0].y;
	splitter_plane.normal.y= splitter_wall.vert_pos[0].x - splitter_wall.vert_pos[1].x;
	const float splitter_wall_square_length= splitter_plane.normal.SquareLength();
	PC_ASSERT( splitter_wall_square_length > 0.0f );
	splitter_plane.normal/= std::sqrt( splitter_wall_square_length );
	splitter_plane.dist= -( splitter_wall.vert_pos[0] * splitter_plane.normal );

	const unsigned int node_number= nodes_.size();
	nodes_.emplace_back();
	Node& node= nodes_.back(); // Reference is valid before recursive calls.

	// Split walls.
	for( const WallsSet::value_type& wall_number : walls )
	{
		PC_ASSERT( wall_number < map_data_->static_walls.size() );
		const MapData::Wall& wall= map_data_->static_walls[ wall_number ];

		float dist0= splitter_plane.GetSignedDistance( wall.vert_pos[0] );
		float dist1= splitter_plane.GetSignedDistance( wall.vert_pos[1] );
		if( wall_number == splitter_wall_number ||
			std::abs(dist0) <= c_plane_dist_eps && std::abs(dist1) <= c_plane_dist_eps)
		{
			// On plane
		}
		else if( dist0 > c_plane_dist_eps && dist1 > c_plane_dist_eps )
		{
			// Front
		}
		else if( dist0 < -c_plane_dist_eps && dist1 < -c_plane_dist_eps )
		{
			// Back
		}
		else
		{
			// splitted
		}
	}

	return node_number;
}

} // namespace PanzerChasm
