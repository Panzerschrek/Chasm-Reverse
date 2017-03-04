#include <matrix.hpp>

#include "../assert.hpp"
#include "../math_utils.hpp"
#include "../game_constants.hpp"
#include "../log.hpp"
#include "../map_loader.hpp"

#include "map_state.hpp"

#include "minimap_state.hpp"

namespace PanzerChasm
{

struct ClipPlane
{
	m_Vec2 normal;
	float dist;

	// if( vertex * normal + dist > 0 ) - behind plane
};

static const float g_z_near= 1.0f / 16.0f;
static const unsigned int g_view_buffer_size= GameConstants::min_screen_width;

static void BuildViewMatrix(
	const m_Vec2& camera_position,
	const float view_angle_z,
	const float fov,
	m_Mat4& out_matrix )
{
	m_Mat4 translate, rotate, perspective, basis_change;
	translate.Translate( m_Vec3( -camera_position, 0.0f ) );
	rotate.RotateZ( -view_angle_z );
	perspective.PerspectiveProjection( 1.0f, fov, g_z_near, 128.0f );

	basis_change.Identity();
	basis_change[5]= 0.0f;
	basis_change[6]= 1.0f;
	basis_change[9]= 1.0f;
	basis_change[10]= 0.0f;

	out_matrix= translate * rotate * basis_change * perspective;
}

// Returns -1, if fully clipped
// Returns +1, if fully accepted
// Returns  0, if splitted
static int ClipLineSegment(
	const ClipPlane& clip_plane,
	m_Vec2& v0, m_Vec2& v1 /* in-out vertices */ )
{
	//return ( rand() % 3 ) - 1;

	float dist[2];
	bool vert_pos[2];

	dist[0]= clip_plane.normal * v0 + clip_plane.dist;
	dist[1]= clip_plane.normal * v1 + clip_plane.dist;
	vert_pos[0]= dist[0] > 0.0f;
	vert_pos[1]= dist[1] > 0.0f;

	if(  vert_pos[0] &&  vert_pos[1] ) return +1;
	if( !vert_pos[0] && !vert_pos[1] ) return -1;

	const float dist_sum= std::abs( dist[0] ) + std::abs( dist[1] );
	if( dist_sum == 0.0f ) return -1;

	const m_Vec2 middle_v= ( std::abs( dist[0] ) * v1 + std::abs( dist[1] ) * v0 ) / dist_sum;

	if( vert_pos[0] )
		v1= middle_v;
	else
		v0= middle_v;

	return 0;
}

MinimapState::MinimapState( const MapDataConstPtr& map_data )
	: map_data_( map_data )
{
	PC_ASSERT( map_data != nullptr );

	static_walls_visibility_.resize( map_data->static_walls.size(), false );
	dynamic_walls_visibility_.resize( map_data->dynamic_walls.size(), false );
}

MinimapState::~MinimapState()
{
}

void MinimapState::SetState(
	const WallsVisibility& static_walls_visibility ,
	const WallsVisibility& dynamic_walls_visibility )
{
	if( static_walls_visibility .size() != static_walls_visibility_ .size() ||
		dynamic_walls_visibility.size() != dynamic_walls_visibility_.size() )
	{
		Log::Warning( "Invalide minimap state" );
		return;
	}

	static_walls_visibility_ = static_walls_visibility ;
	dynamic_walls_visibility_= dynamic_walls_visibility;
}

void MinimapState::Update(
	const MapState& map_state,
	const m_Vec2& camera_position,
	const float view_angle_z )
{
	// TODO - use real field of view.
	const float c_fov= Constants::half_pi;

	// Setup buffer
	typedef float DepthType;
	MapData::IndexElement screen_buffer[ g_view_buffer_size ];
	DepthType depth_buffer[ g_view_buffer_size ];

	for( unsigned int i= 0u; i < g_view_buffer_size; i++ )
	{
		screen_buffer[i].type= MapData::IndexElement::None;
		depth_buffer[i]= 1.0f;
	}

	// Create view matrix
	m_Mat4 view_matrix;
	BuildViewMatrix( camera_position, view_angle_z, c_fov, view_matrix );

	// Setup clip planes
	const unsigned int c_clip_planes= 3u;
	ClipPlane clip_planes[ c_clip_planes ];
	{
		const float c_rot= Constants::half_pi; // I Don`t know why, but this needs.
		const float plane_1_angle= c_rot + view_angle_z + ( Constants::half_pi - 0.5f * c_fov );
		const float plane_2_angle= c_rot + view_angle_z - ( Constants::half_pi - 0.5f * c_fov );

		clip_planes[0].normal= m_Vec2( std::cos( c_rot + view_angle_z ), std::sin( c_rot + view_angle_z ) );
		clip_planes[0].dist= -( clip_planes[0].normal * camera_position + g_z_near );

		clip_planes[1].normal= m_Vec2( std::cos( plane_1_angle ), std::sin( plane_1_angle ) );
		clip_planes[1].dist= -( clip_planes[1].normal * camera_position );

		clip_planes[2].normal= m_Vec2( std::cos( plane_2_angle ), std::sin( plane_2_angle ) );
		clip_planes[2].dist= -( clip_planes[2].normal * camera_position );
	}

	const auto draw_wall=
	[&]( const m_Vec2& v0, const m_Vec2& v1, const MapData::IndexElement& wall_index )
	{
		m_Vec2 v0_clipped= v0;
		m_Vec2 v1_clipped= v1;

		for( unsigned int p= 0u; p < 3u; p++ )
		{
			const int clip_result= ClipLineSegment( clip_planes[p], v0_clipped, v1_clipped );
			if( clip_result == -1 )
				return;
		}

		m_Vec3 v0_projected= m_Vec3( v0_clipped, 0.0f ) * view_matrix;
		m_Vec3 v1_projected= m_Vec3( v1_clipped, 0.0f ) * view_matrix;
		// TODO - move W calculation to function.
		const float w0= v0_clipped.x * view_matrix.value[3] + v0_clipped.y * view_matrix.value[7] + view_matrix.value[15];
		const float w1= v1_clipped.x * view_matrix.value[3] + v1_clipped.y * view_matrix.value[7] + view_matrix.value[15];
		PC_ASSERT( w0 > 0.0f );
		PC_ASSERT( w1 > 0.0f );
		v0_projected/= w0;
		v1_projected/= w1;

		float v0_depth, v1_depth;
		float v0_x= ( v0_projected.x + 1.0f ) * ( 0.5f * float(g_view_buffer_size) );
		float v1_x= ( v1_projected.x + 1.0f ) * ( 0.5f * float(g_view_buffer_size) );
		if( v0_x < v1_x )
		{
			v0_depth= v0_projected.z;
			v1_depth= v1_projected.z;
		}
		else if( v0_x > v1_x )
		{
			std::swap( v0_x, v1_x );
			v0_depth= v1_projected.z;
			v1_depth= v0_projected.z;
		}
		else
			return;

		const float d_depth_dx= ( v1_depth - v0_depth ) / ( v1_x - v0_x );
		const int x_start= std::max( 0, static_cast<int>(std::round(v0_x)) );
		const int x_end= std::min( static_cast<int>(std::round(v1_x)), int(g_view_buffer_size) );
		const float depth_start= v0_depth + ( float(x_start) + 0.5f - v0_x ) * d_depth_dx;
		for( int x= x_start; x < x_end; x++ )
		{
			const float depth= depth_start + float( x - x_start ) * d_depth_dx;
			if( depth < depth_buffer[x] )
			{
				depth_buffer[x]= depth;
				screen_buffer[x]= wall_index;
			}
		}
	};

	// Draw static walls
	for( const MapData::Wall& wall : map_data_->static_walls )
	{
		if( wall.texture_id >= MapData::c_first_transparent_texture_id )
			continue;

		MapData::IndexElement index;
		index.type= MapData::IndexElement::StaticWall;
		index.index= &wall - map_data_->static_walls.data();
		draw_wall( wall.vert_pos[0], wall.vert_pos[1], index );
	}

	// Draw dynamic walls
	const MapState::DynamicWalls& dynamic_walls= map_state.GetDynamicWalls();
	for( const MapState::DynamicWall& wall : dynamic_walls )
	{
		if( wall.texture_id >= MapData::c_first_transparent_texture_id )
			continue;

		MapData::IndexElement index;
		index.type= MapData::IndexElement::DynamicWall;
		index.index= &wall - dynamic_walls.data();
		draw_wall( wall.vert_pos[0], wall.vert_pos[1], index );
	}

	// Update visibility.
	for( const MapData::IndexElement& index_element : screen_buffer )
	{
		if( index_element.type == MapData::IndexElement::StaticWall )
			static_walls_visibility_[ index_element.index ]= true;
		else if( index_element.type == MapData::IndexElement::DynamicWall )
			dynamic_walls_visibility_[ index_element.index ]= true;
	}
}

const MinimapState::WallsVisibility& MinimapState::GetStaticWallsVisibility() const
{
	return static_walls_visibility_;
}

const MinimapState::WallsVisibility& MinimapState::GetDynamicWallsVisibility() const
{
	return dynamic_walls_visibility_;
}

} // namespace PanzerChasm
