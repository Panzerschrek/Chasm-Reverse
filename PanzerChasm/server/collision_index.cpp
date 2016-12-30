#include "../assert.hpp"

#include "collision_index.hpp"

namespace PanzerChasm
{

CollisionIndex::CollisionIndex( const MapDataConstPtr& map_data )
{
	PC_ASSERT( map_data != nullptr );

	for( unsigned short& i : index_field_ )
		i= IndexElement::c_dummy_next;

	for( const MapData::Wall& wall : map_data->static_walls )
	{
		if( wall.vert_pos[0] == wall.vert_pos[1] )
			continue;

		MapData::IndexElement wall_index_element;
		wall_index_element.type= MapData::IndexElement::StaticWall;
		wall_index_element.index= &wall - map_data->static_walls.data();

		const m_Vec2 dir= wall.vert_pos[1] - wall.vert_pos[0];
		if( std::abs( dir.x ) >= std::abs( dir.y ) )
		{
			// X Major
			m_Vec2 v[2];
			if( dir.x > 0.0f )
			{
				v[0]= wall.vert_pos[0];
				v[1]= wall.vert_pos[1];
			}
			else
			{
				v[0]= wall.vert_pos[1];
				v[1]= wall.vert_pos[0];
			}

			const float dy_dx= ( v[1].y - v[0].y ) / ( v[1].x - v[0].x );

			const int start_x= std::max( static_cast<int>( std::floor( v[0].x ) ), 0 );
			const int   end_x= std::min( static_cast<int>( std::floor( v[1].x ) ), int(MapData::c_map_size - 1u) );

			for( int x= start_x; x <= end_x; x++ )
			{
				const float cell_x_start= std::max( float(x  ), v[0].x );
				const float cell_x_end  = std::min( float(x+1), v[1].x );

				const float cell_y_start= v[0].y + dy_dx * ( cell_x_start - v[0].x );
				const float cell_y_end  = v[0].y + dy_dx * ( cell_x_end   - v[0].x );
				const int cell_y_start_i= std::max( static_cast<int>( std::floor( cell_y_start ) ), 0 );
				const int cell_y_end_i  = std::min( static_cast<int>( std::floor( cell_y_end   ) ), int(MapData::c_map_size - 1u) );
				for( int y= cell_y_start_i; y <= cell_y_end_i; y++ )
					AddElementToIndex( x, y, wall_index_element );
			}
		}
		else
		{
			// Y Major
			m_Vec2 v[2];
			if( dir.y > 0.0f )
			{
				v[0]= wall.vert_pos[0];
				v[1]= wall.vert_pos[1];
			}
			else
			{
				v[0]= wall.vert_pos[1];
				v[1]= wall.vert_pos[0];
			}

			const float dx_dy= ( v[1].x - v[0].x ) / ( v[1].y - v[0].y );

			const int start_y= std::max( static_cast<int>( std::floor( v[0].y ) ), 0 );
			const int   end_y= std::min( static_cast<int>( std::floor( v[1].y ) ), int(MapData::c_map_size - 1u) );

			for( int y= start_y; y <= end_y; y++ )
			{
				const float cell_y_start= std::max( float(y  ), v[0].y );
				const float cell_y_end  = std::min( float(y+1), v[1].y );

				const float cell_x_start= v[0].x + dx_dy * ( cell_y_start - v[0].y );
				const float cell_x_end  = v[0].x + dx_dy * ( cell_y_end   - v[0].y );
				const int cell_x_start_i= std::max( static_cast<int>( std::floor( cell_x_start ) ), 0 );
				const int cell_x_end_i  = std::min( static_cast<int>( std::floor( cell_x_end   ) ), int(MapData::c_map_size - 1u) );
				for( int x= cell_x_start_i; x <= cell_x_end_i; x++ )
					AddElementToIndex( x, y, wall_index_element );
			}
		}
	} // for static walls
}

CollisionIndex::~CollisionIndex()
{}

void CollisionIndex::AddElementToIndex( unsigned int x, unsigned int y, const MapData::IndexElement& element )
{
	PC_ASSERT( x < MapData::c_map_size );
	PC_ASSERT( y < MapData::c_map_size );

	index_elements_.emplace_back();
	IndexElement& index_element= index_elements_.back();
	index_element.index_element= element;

	const unsigned short current_xy_element= index_field_[ x + y * MapData::c_map_size ];
	if( current_xy_element == IndexElement::c_dummy_next )
		index_element.next= IndexElement::c_dummy_next;
	else
		index_element.next= current_xy_element;

	index_field_[ x + y * MapData::c_map_size ]= index_elements_.size() - 1u;
}

} // namespace PanzerChasm
