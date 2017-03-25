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

	BuildSegments segments;
	segments.resize( map_data_->static_walls.size() );
	for( unsigned int i= 0u; i < segments.size(); i++ )
	{
		const MapData::Wall& wall= map_data_->static_walls[i];
		BuildSegment& segment= segments[i];
		segment.wall_index= i;

		segment.vert_pos[0]= wall.vert_pos[0];
		segment.vert_pos[1]= wall.vert_pos[1];
	}

	root_node_= BuildTree_r( segments );
}

MapBSPTree::~MapBSPTree()
{
}

unsigned int MapBSPTree::BuildTree_r( const BuildSegments& build_segments )
{
	PC_ASSERT( !build_segments.empty() );

	const float c_plane_dist_eps= 1.0f / 256.0f;

	int best_score= std::numeric_limits<int>::max();
	const BuildSegment* best_splitter_segment= nullptr;

	// Select best splitter.
	// TODO - if we have too much walls - check not all walls, but some percent of random walls.
	for( const BuildSegment& splitter_segment : build_segments )
	{
		m_Plane2 plane;
		plane.normal.x= splitter_segment.vert_pos[1].y - splitter_segment.vert_pos[0].y;
		plane.normal.y= splitter_segment.vert_pos[0].x - splitter_segment.vert_pos[1].x;
		const float segment_square_length= plane.normal.SquareLength();
		if( segment_square_length <= 0.0f )
			continue;

		plane.normal/= std::sqrt( segment_square_length );
		plane.dist= -( splitter_segment.vert_pos[0] * plane.normal );

		int on_plane_count= 0, splitted_count= 0, front_count= 0, back_count= 0;
		for( const BuildSegment& segment : build_segments )
		{
			// Force mark splitter as wall on plane. Reduce calculation errors.
			if( &segment == &splitter_segment )
			{
				on_plane_count++;
				continue;
			}

			float dist0= plane.GetSignedDistance( segment.vert_pos[0] );
			float dist1= plane.GetSignedDistance( segment.vert_pos[1] );
			if( std::abs(dist0) <= c_plane_dist_eps && std::abs(dist1) <= c_plane_dist_eps ) // Fully on plane.
				on_plane_count++;
			else if( dist0 > +c_plane_dist_eps && dist1 > +c_plane_dist_eps ) // Fully front.
				front_count++;
			else if( dist0 < -c_plane_dist_eps && dist1 < -c_plane_dist_eps ) // Fully back.
				back_count++;
			else if( dist0 > +c_plane_dist_eps && std::abs(dist1) <= c_plane_dist_eps ) // Point on plane + front.
				front_count++;
			else if( dist1 > +c_plane_dist_eps && std::abs(dist0) <= c_plane_dist_eps ) // Point on plane + front.
				front_count++;
			else if( dist0 < -c_plane_dist_eps && std::abs(dist1) <= c_plane_dist_eps ) // Point on plane +  back.
				 back_count++;
			else if( dist1 < -c_plane_dist_eps && std::abs(dist0) <= c_plane_dist_eps ) // Point on plane +  back.
				 back_count++;
			else
				splitted_count++;
		}

		// TODO - calibrate score formula. Maybe, substract some score for walls along X or Y axis?
		// score - smaller means better.
		int score= std::abs( front_count - back_count ) - 2 * on_plane_count + 2 * splitted_count;
		if( score < best_score )
		{
			best_score= score;
			best_splitter_segment= &splitter_segment;
		}
	} // fo splitter candidates.

	if( best_splitter_segment == nullptr )
	{
		PC_ASSERT( false );
		return c_null_node;
	}

	m_Plane2 splitter_plane;
	splitter_plane.normal.x= best_splitter_segment->vert_pos[1].y - best_splitter_segment->vert_pos[0].y;
	splitter_plane.normal.y= best_splitter_segment->vert_pos[0].x - best_splitter_segment->vert_pos[1].x;
	const float splitter_segment_square_length= splitter_plane.normal.SquareLength();
	PC_ASSERT( splitter_segment_square_length > 0.0f );
	splitter_plane.normal/= std::sqrt( splitter_segment_square_length );
	splitter_plane.dist= -( best_splitter_segment->vert_pos[0] * splitter_plane.normal );

	const unsigned int node_number= nodes_.size();
	nodes_.emplace_back();
	Node* node= &nodes_[node_number]; // Pointer is valid before recursive calls.
	node->first_segment= segments_.size();
	node->segment_count= 0u;
	node->plane= splitter_plane;

	// Split input segments.
	BuildSegments front_segments;
	BuildSegments  back_segments;

	for( const BuildSegment& segment : build_segments )
	{
		float dist0= splitter_plane.GetSignedDistance( segment.vert_pos[0] );
		float dist1= splitter_plane.GetSignedDistance( segment.vert_pos[1] );
		if( &segment == best_splitter_segment ||
			( std::abs(dist0) <= c_plane_dist_eps && std::abs(dist1) <= c_plane_dist_eps ) )
		{
			// On plane
			// Generate result segment.
			segments_.emplace_back();
			WallSegment& out_segment= segments_.back();
			out_segment.wall_index= segment.wall_index;
			out_segment.vert_pos[0]= segment.vert_pos[0];
			out_segment.vert_pos[1]= segment.vert_pos[1];

			PC_ASSERT( segment.wall_index < map_data_->static_walls.size() );
			const MapData::Wall& wall= map_data_->static_walls[ segment.wall_index ];
			const float wall_length= ( wall.vert_pos[1] - wall.vert_pos[0] ).Length();

			// TODO - check this.
			if( wall_length > 0.0f )
			{
				out_segment.start= ( segment.vert_pos[0] - wall.vert_pos[0] ).Length() / wall_length;
				out_segment.end  = ( segment.vert_pos[1] - wall.vert_pos[0] ).Length() / wall_length;
			}
			else
			{
				out_segment.start= 0.0f;
				out_segment.end= 1.0f;
			}
			if( out_segment.start > out_segment.end )
			{
				std::swap( out_segment.start, out_segment.end );
				std::swap( out_segment.vert_pos[0], out_segment.vert_pos[1] );
			}

			node->segment_count++;
		}
		else if( dist0 > +c_plane_dist_eps && dist1 > +c_plane_dist_eps ) // Fully front.
			front_segments.push_back( segment );
		else if( dist0 < -c_plane_dist_eps && dist1 < -c_plane_dist_eps ) // Fully  back.
			 back_segments.push_back( segment );
		else if( dist0 > +c_plane_dist_eps && std::abs(dist1) <= c_plane_dist_eps ) // Point on plane + front.
			front_segments.push_back( segment );
		else if( dist1 > +c_plane_dist_eps && std::abs(dist0) <= c_plane_dist_eps ) // Point on plane + front.
			front_segments.push_back( segment );
		else if( dist0 < -c_plane_dist_eps && std::abs(dist1) <= c_plane_dist_eps ) // Point on plane +  back.
			 back_segments.push_back( segment );
		else if( dist1 < -c_plane_dist_eps && std::abs(dist0) <= c_plane_dist_eps ) // Point on plane +  back.
			 back_segments.push_back( segment );
		else
		{
			front_segments.emplace_back();
			 back_segments.emplace_back();
			BuildSegment& new_front_segment= front_segments.back();
			BuildSegment&  new_back_segment=  back_segments.back();
			new_front_segment.wall_index= new_back_segment.wall_index= segment.wall_index;

			// Splitted segment.
			if( dist0 > dist1 )
			{
				const float dist_sum= dist0 - dist1;
				const float k0=   dist0  / dist_sum;
				const float k1= (-dist1) / dist_sum;
				PC_ASSERT( dist_sum >= 0.0f );
				PC_ASSERT( k0 >= 0.0f );
				PC_ASSERT( k1 >= 0.0f );

				const m_Vec2 middle_point= k0 * segment.vert_pos[1] + k1 * segment.vert_pos[0];
				new_front_segment.vert_pos[0]= segment.vert_pos[0];
				new_front_segment.vert_pos[1]= middle_point;
				 new_back_segment.vert_pos[0]= middle_point;
				 new_back_segment.vert_pos[1]= segment.vert_pos[1];
			}
			else
			{
				const float dist_sum= dist1 - dist0;
				const float k0= (-dist0) / dist_sum;
				const float k1=   dist1  / dist_sum;
				PC_ASSERT( dist_sum >= 0.0f );
				PC_ASSERT( k0 >= 0.0f );
				PC_ASSERT( k1 >= 0.0f );

				const m_Vec2 middle_point= k0 * segment.vert_pos[1] + k1 * segment.vert_pos[0];
				new_front_segment.vert_pos[0]= middle_point;
				new_front_segment.vert_pos[1]= segment.vert_pos[1];
				 new_back_segment.vert_pos[0]= segment.vert_pos[0];
				 new_back_segment.vert_pos[1]= middle_point;
			}
		} // if segment is splitted.
	} // for segments

	const unsigned int node_front= front_segments.empty() ? c_null_node : BuildTree_r( front_segments );
	const unsigned int node_back =  back_segments.empty() ? c_null_node : BuildTree_r(  back_segments );

	node= &nodes_[node_number]; // Update pointer after recursive calls.
	node->node_front= node_front;
	node->node_back= node_back;

	return node_number;
}

} // namespace PanzerChasm
