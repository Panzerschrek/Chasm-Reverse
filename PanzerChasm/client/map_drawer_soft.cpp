#include "../assert.hpp"
#include "../game_constants.hpp"
#include "../log.hpp"
#include "../map_loader.hpp"
#include "../math_utils.hpp"
#include "map_drawers_common.hpp"

#include "map_drawer_soft.hpp"

namespace PanzerChasm
{

MapDrawerSoft::MapDrawerSoft(
	Settings& settings,
	const GameResourcesConstPtr& game_resources,
	const RenderingContextSoft& rendering_context )
	: game_resources_( game_resources )
	, rendering_context_( rendering_context )
	, screen_transform_x_( 0.5f * float( rendering_context_.viewport_size.Width () ) )
	, screen_transform_y_( 0.5f * float( rendering_context_.viewport_size.Height() ) )
	, rasterizer_(
		rendering_context.viewport_size.Width(), rendering_context.viewport_size.Height(),
		rendering_context.row_pixels, rendering_context.window_surface_data )
{
	PC_ASSERT( game_resources_ != nullptr );

	// TODO
	PC_UNUSED( settings );

	LoadModelsGroup( game_resources_->items_models, items_models_ );
	LoadModelsGroup( game_resources_->rockets_models, rockets_models_ );
	LoadModelsGroup( game_resources_->weapons_models, weapons_models_ );
	LoadModelsGroup( game_resources_->monsters_models, monsters_models_ );
}

MapDrawerSoft::~MapDrawerSoft()
{}

void MapDrawerSoft::SetMap( const MapDataConstPtr& map_data )
{
	current_map_data_= map_data;
	if( map_data == nullptr )
		return; // TODO - if map is null - clear resources, etc.

	LoadModelsGroup( map_data->models, map_models_ );
	LoadWallsTextures( *map_data );
	LoadFloorsTextures( *map_data );
	LoadFloorsAndCeilings( *map_data );
}

void MapDrawerSoft::Draw(
	const MapState& map_state,
	const m_Mat4& view_rotation_and_projection_matrix,
	const m_Vec3& camera_position,
	const ViewClipPlanes& view_clip_planes,
	const EntityId player_monster_id )
{
	PC_UNUSED( player_monster_id );

	if( current_map_data_ == nullptr )
		return;

	rasterizer_.ClearDepthBuffer();

	m_Mat4 cam_shift_mat, cam_mat, screen_flip_mat;
	cam_shift_mat.Translate( -camera_position );
	screen_flip_mat.Scale( m_Vec3( 1.0f, -1.0f, 1.0f ) );
	cam_mat= cam_shift_mat * view_rotation_and_projection_matrix * screen_flip_mat;

	DrawWalls( map_state, cam_mat, camera_position.xy(), view_clip_planes );
	DrawFloorsAndCeilings( cam_mat, view_clip_planes );

	for( const MapState::StaticModel& static_model : map_state.GetStaticModels() )
	{
		if( static_model.model_id >= current_map_data_->models_description.size() )
			continue;

		m_Mat4 rotate_mat;
		rotate_mat.RotateZ( static_model.angle );

		DrawModel(
			map_models_, current_map_data_->models, static_model.model_id,
			static_model.animation_frame,
			view_clip_planes,
			static_model.pos,
			rotate_mat,
			cam_mat );
	}

	for( const MapState::Item& item : map_state.GetItems() )
	{
		if( item.item_id >= game_resources_->items_models.size() ||
			item.picked_up )
			continue;

		m_Mat4 rotate_mat;
		rotate_mat.RotateZ( item.angle );

		DrawModel(
			items_models_, game_resources_->items_models, item.item_id,
			item.animation_frame,
			view_clip_planes,
			item.pos,
			rotate_mat,
			cam_mat );
	}

	for( const MapState::DynamicItemsContainer::value_type& dynamic_item_value : map_state.GetDynamicItems() )
	{
		const MapState::DynamicItem& item= dynamic_item_value.second;
		if( item.item_type_id >= game_resources_->items_models.size() )
			continue;

		m_Mat4 rotate_mat;
		rotate_mat.RotateZ( item.angle );

		DrawModel(
			items_models_, game_resources_->items_models, item.item_type_id,
			item.frame,
			view_clip_planes,
			item.pos,
			rotate_mat,
			cam_mat );
	}

	for( const MapState::RocketsContainer::value_type& rocket_value : map_state.GetRockets() )
	{
		const MapState::Rocket& rocket= rocket_value.second;
		if( rocket.rocket_id >= game_resources_->rockets_models.size() )
			continue;

		m_Mat4 rotate_max_x, rotate_mat_z;
		rotate_max_x.RotateX( rocket.angle[1] );
		rotate_mat_z.RotateZ( rocket.angle[0] - Constants::half_pi );

		DrawModel(
			rockets_models_, game_resources_->rockets_models, rocket.rocket_id,
			rocket.frame,
			view_clip_planes,
			rocket.pos,
			rotate_max_x * rotate_mat_z,
			cam_mat );
	}

	for( const MapState::MonstersContainer::value_type& monster_value : map_state.GetMonsters() )
	{
		const MapState::Monster& monster= monster_value.second;
		if( monster.monster_id >= game_resources_->monsters_models.size() )
			continue;

		if( monster_value.first == player_monster_id )
			continue;

		const unsigned int frame=
			game_resources_->monsters_models[ monster.monster_id ].animations[ monster.animation ].first_frame +
			monster.animation_frame;

		m_Mat4 rotate_mat;
		rotate_mat.RotateZ( monster.angle + Constants::half_pi );

		DrawModel(
			monsters_models_, game_resources_->monsters_models, monster.monster_id,
			frame,
			view_clip_planes,
			monster.pos,
			rotate_mat,
			cam_mat );
	}
}

void MapDrawerSoft::DrawWeapon(
	const WeaponState& weapon_state,
	const m_Mat4& projection_matrix,
	const m_Vec3& camera_position,
	const float x_angle, const float z_angle )
{
	// TODO
	PC_UNUSED( weapon_state );
	PC_UNUSED( projection_matrix );
	PC_UNUSED( camera_position );
	PC_UNUSED( x_angle );
	PC_UNUSED( z_angle );
}

void MapDrawerSoft::LoadModelsGroup( const std::vector<Model>& models, ModelsGroup& out_group )
{
	const PaletteTransformed& palette= *rendering_context_.palette_transformed;

	unsigned int texel_count= 0u;
	for( const Model& model : models )
		texel_count+= model.texture_data.size();

	out_group.textures_data.resize( texel_count );
	out_group.models.resize( models.size() );

	unsigned int texture_data_offset= 0u;
	for( unsigned int m= 0u; m < out_group.models.size(); m++ )
	{
		const Model& in_model= models[m];
		ModelsGroup::ModelEntry& model_entry= out_group.models[m];

		model_entry.texture_size[0]= in_model.texture_size[0];
		model_entry.texture_size[1]= in_model.texture_size[1];
		model_entry.texture_data_offset= texture_data_offset;

		for( unsigned int t= 0u; t < in_model.texture_data.size(); t++ )
			out_group.textures_data[ texture_data_offset + t ]= palette[ in_model.texture_data[t] ];

		texture_data_offset+= in_model.texture_data.size();
	}
}

void MapDrawerSoft::LoadWallsTextures( const MapData& map_data )
{
	const PaletteTransformed& palette= *rendering_context_.palette_transformed;

	std::vector<unsigned char> file_content;

	for( unsigned int i= 0u; i < MapData::c_max_walls_textures; i++ )
	{
		WallTexture& out_texture= wall_textures_[i];
		out_texture.size[0]= out_texture.size[1]= 0u;

		const char* const texture_file_path= map_data.walls_textures[i].file_path;
		if( texture_file_path[0] == '\n' )
			continue;

		game_resources_->vfs->ReadFile( texture_file_path, file_content );
		if( file_content.empty() )
			continue;

		const CelTextureHeader& header= *reinterpret_cast<const CelTextureHeader*>( file_content.data() );
		if( g_max_wall_texture_width / header.size[0] * header.size[0] != g_max_wall_texture_width ||
			header.size[1] != g_wall_texture_height )
		{
			Log::Warning( "Invalid wall texture size: ", header.size[0], "x", header.size[1] );
			continue;
		}

		out_texture.size[0]= header.size[0];
		out_texture.size[1]= header.size[1];

		const unsigned int pixel_count= header.size[0] * header.size[1];
		const unsigned char* const src= file_content.data() + sizeof(CelTextureHeader);

		out_texture.data.resize( pixel_count );
		for( unsigned int j= 0u; j < pixel_count; j++ )
			out_texture.data[j]= palette[ src[j] ];

		// Calculate top and bottom alpha-rejected texture rows.
		out_texture.full_alpha_row[0]= 0u;
		out_texture.full_alpha_row[1]= g_wall_texture_height;

		for( unsigned int y= 0u; y < header.size[1]; y++ )
		{
			bool is_full_alpha= true;
			for( unsigned int x= 0u; x < header.size[0]; x++ )
				if( src[x + y * header.size[0] ] != 255u )
				{
					is_full_alpha= false;
					break;
				}

			if( !is_full_alpha )
			{
				out_texture.full_alpha_row[0]= y;
				break;
			}
		}
		for( unsigned int y= 0u; y < header.size[1]; y++ )
		{
			bool is_full_alpha= true;
			for( unsigned int x= 0u; x < header.size[0]; x++ )
				if( src[ x + ( header.size[1] - 1u - y ) * header.size[0] ] != 255u )
				{
					is_full_alpha= false;
					break;
				}

			if( !is_full_alpha )
			{
				out_texture.full_alpha_row[1]= header.size[1] - y;
				break;
			}
		}
	}
}

void MapDrawerSoft::LoadFloorsTextures( const MapData& map_data )
{
	const PaletteTransformed& palette= *rendering_context_.palette_transformed;

	for( unsigned int i= 0u; i < MapData::c_floors_textures_count; i++ )
	{
		const unsigned char* const src= map_data.floor_textures_data[i];
		uint32_t* const dst= floor_textures_[i].data;
		for( unsigned int j= 0u; j < MapData::c_floor_texture_size * MapData::c_floor_texture_size; j++ )
			dst[j]= palette[ src[j] ];
	}
}

void MapDrawerSoft::LoadFloorsAndCeilings( const MapData& map_data )
{
	map_floors_and_ceilings_.clear();

	for( unsigned int i= 0u; i < 2u; i++ )
	{
		( i == 0u ? first_floor_ : first_ceiling_ )= map_floors_and_ceilings_.size();

		const unsigned char* const src= i == 0u ? map_data.floor_textures : map_data.ceiling_textures;

		for( unsigned int y= 0u; y < MapData::c_map_size; y++ )
		for( unsigned int x= 0u; x < MapData::c_map_size; x++ )
		{
			const unsigned char texture_number= src[ x + y * MapData::c_map_size ];

			if( texture_number == MapData::c_empty_floor_texture_id ||
				texture_number == MapData::c_sky_floor_texture_id ||
				texture_number >= MapData::c_floors_textures_count )
				continue;

			map_floors_and_ceilings_.emplace_back();
			FloorCeilingCell& cell= map_floors_and_ceilings_.back();
			cell.xy[0]= x;
			cell.xy[1]= y;
			cell.texture_id= texture_number;
		}
	}
}

template<class WallsContainer>
void MapDrawerSoft::DrawWallsImpl( const WallsContainer& walls, const m_Mat4& matrix, const m_Vec2& camera_position_xy, const ViewClipPlanes& view_clip_planes )
{
	for( const auto& wall : walls )
	{
		PC_ASSERT( wall.texture_id < MapData::c_max_walls_textures );

		const WallTexture& texture= wall_textures_[ wall.texture_id ];
		if( texture.size[0] == 0u || texture.size[1] == 0u )
			continue;
		if( texture.full_alpha_row[0] == texture.full_alpha_row[1] )
			continue;

		// Discard back faces.
		if( wall.texture_id < MapData::c_first_transparent_texture_id &&
			mVec2Cross( camera_position_xy - wall.vert_pos[0], wall.vert_pos[1] - wall.vert_pos[0] ) > 0.0f )
			continue;

		const float z_bottom_top[]=
		{
			GameConstants::walls_height - float(texture.full_alpha_row[0]) * ( GameConstants::walls_height / float(g_wall_texture_height) ),
			GameConstants::walls_height - float(texture.full_alpha_row[1]) * ( GameConstants::walls_height / float(g_wall_texture_height) )
		};

		clipped_vertices_[0].pos= m_Vec3( wall.vert_pos[0], z_bottom_top[0] );
		clipped_vertices_[1].pos= m_Vec3( wall.vert_pos[0], z_bottom_top[1] );
		clipped_vertices_[2].pos= m_Vec3( wall.vert_pos[1], z_bottom_top[1] );
		clipped_vertices_[3].pos= m_Vec3( wall.vert_pos[1], z_bottom_top[0] );
		clipped_vertices_[0].tc= m_Vec2( 0.0f, float( texture.full_alpha_row[0] << 16u ) );
		clipped_vertices_[1].tc= m_Vec2( 0.0f, float( texture.full_alpha_row[1] << 16u ) );
		clipped_vertices_[2].tc= m_Vec2( float(texture.size[0] << 16u ), float( texture.full_alpha_row[1] << 16u ) );
		clipped_vertices_[3].tc= m_Vec2( float(texture.size[0] << 16u ), float( texture.full_alpha_row[0] << 16u ) );
		clipped_vertices_[0].next= &clipped_vertices_[1];
		clipped_vertices_[1].next= &clipped_vertices_[2];
		clipped_vertices_[2].next= &clipped_vertices_[3];
		clipped_vertices_[3].next= &clipped_vertices_[0];
		fisrt_clipped_vertex_= &clipped_vertices_[0];
		next_new_clipped_vertex_= 4u;

		unsigned int polygon_vertex_count= 4u;
		for( const m_Plane3& plane : view_clip_planes )
		{
			polygon_vertex_count= ClipPolygon( plane, polygon_vertex_count );
			PC_ASSERT( polygon_vertex_count == 0u || polygon_vertex_count >= 3u );
			if( polygon_vertex_count == 0u )
				break;
		}
		if( polygon_vertex_count == 0u )
			continue;

		RasterizerVertex verties_projected[ c_max_clip_vertices_ ];
		ClippedVertex* v= fisrt_clipped_vertex_;
		for( unsigned int i= 0u; i < polygon_vertex_count; i++, v= v->next )
		{
			m_Vec3 vertex_projected= v->pos * matrix;
			const float w= v->pos.x * matrix.value[3] + v->pos.y * matrix.value[7] + v->pos.z * matrix.value[11] + matrix.value[15];

			vertex_projected/= w;
			vertex_projected.z= w;

			vertex_projected.x= ( vertex_projected.x + 1.0f ) * screen_transform_x_;
			vertex_projected.y= ( vertex_projected.y + 1.0f ) * screen_transform_y_;

			RasterizerVertex& out_v= verties_projected[ i ];
			out_v.x= fixed16_t( vertex_projected.x * 65536.0f );
			out_v.y= fixed16_t( vertex_projected.y * 65536.0f );
			out_v.u= fixed16_t( v->tc.x );
			out_v.v= fixed16_t( v->tc.y );
			out_v.z= fixed16_t( w * 65536.0f );
		}

		rasterizer_.SetTexture( texture.size[0], texture.size[1], texture.data.data() );

		RasterizerVertex traingle_vertices[3];
		traingle_vertices[0]= verties_projected[0];
		for( unsigned int i= 0u; i < polygon_vertex_count - 2u; i++ )
		{
			traingle_vertices[1]= verties_projected[ i + 1u ];
			traingle_vertices[2]= verties_projected[ i + 2u ];
			rasterizer_.DrawTexturedTriangleSpanCorrected( traingle_vertices );
		}
	}
}

void MapDrawerSoft::DrawWalls(
	const MapState& map_state,
	const m_Mat4& matrix,
	const m_Vec2& camera_position_xy,
	const ViewClipPlanes& view_clip_planes )
{
	DrawWallsImpl( current_map_data_->static_walls, matrix, camera_position_xy, view_clip_planes );
	DrawWallsImpl( map_state.GetDynamicWalls(), matrix, camera_position_xy, view_clip_planes );
}

void MapDrawerSoft::DrawFloorsAndCeilings( const m_Mat4& matrix, const ViewClipPlanes& view_clip_planes  )
{
	for( unsigned int i= 0u; i < map_floors_and_ceilings_.size(); i++ )
	{
		const FloorCeilingCell& cell= map_floors_and_ceilings_[i];
		const float z= i < first_ceiling_ ? 0.0f : GameConstants::walls_height;

		PC_ASSERT( cell.texture_id < MapData::c_floors_textures_count );

		clipped_vertices_[0].pos= m_Vec3( float(cell.xy[0]   ), float(cell.xy[1]   ), z );
		clipped_vertices_[1].pos= m_Vec3( float(cell.xy[0]+1u), float(cell.xy[1]   ), z );
		clipped_vertices_[2].pos= m_Vec3( float(cell.xy[0]+1u), float(cell.xy[1]+1u), z );
		clipped_vertices_[3].pos= m_Vec3( float(cell.xy[0]   ), float(cell.xy[1]+1u), z );
		clipped_vertices_[0].tc= m_Vec2( 0.0f, 0.0f );
		clipped_vertices_[1].tc= m_Vec2( float( MapData::c_floor_texture_size << 16u ), 0.0f );
		clipped_vertices_[2].tc= m_Vec2( float( MapData::c_floor_texture_size << 16u ), float( MapData::c_floor_texture_size << 16u ) );
		clipped_vertices_[3].tc= m_Vec2( 0.0f, float( MapData::c_floor_texture_size << 16u ) );
		clipped_vertices_[0].next= &clipped_vertices_[1];
		clipped_vertices_[1].next= &clipped_vertices_[2];
		clipped_vertices_[2].next= &clipped_vertices_[3];
		clipped_vertices_[3].next= &clipped_vertices_[0];
		fisrt_clipped_vertex_= &clipped_vertices_[0];
		next_new_clipped_vertex_= 4u;

		unsigned int polygon_vertex_count= 4u;
		for( const m_Plane3& plane : view_clip_planes )
		{
			polygon_vertex_count= ClipPolygon( plane, polygon_vertex_count );
			PC_ASSERT( polygon_vertex_count == 0u || polygon_vertex_count >= 3u );
			if( polygon_vertex_count == 0u )
				break;
		}
		if( polygon_vertex_count == 0u )
			continue;

		RasterizerVertex verties_projected[ c_max_clip_vertices_ ];
		ClippedVertex* v= fisrt_clipped_vertex_;
		for( unsigned int i= 0u; i < polygon_vertex_count; i++, v= v->next )
		{
			m_Vec3 vertex_projected= v->pos * matrix;
			const float w= v->pos.x * matrix.value[3] + v->pos.y * matrix.value[7] + v->pos.z * matrix.value[11] + matrix.value[15];

			vertex_projected/= w;
			vertex_projected.z= w;

			vertex_projected.x= ( vertex_projected.x + 1.0f ) * screen_transform_x_;
			vertex_projected.y= ( vertex_projected.y + 1.0f ) * screen_transform_y_;

			RasterizerVertex& out_v= verties_projected[ i ];
			out_v.x= fixed16_t( vertex_projected.x * 65536.0f );
			out_v.y= fixed16_t( vertex_projected.y * 65536.0f );
			out_v.u= fixed16_t( v->tc.x );
			out_v.v= fixed16_t( v->tc.y );
			out_v.z= fixed16_t( w * 65536.0f );
		}

		rasterizer_.SetTexture(
			MapData::c_floor_texture_size, MapData::c_floor_texture_size,
			floor_textures_[ cell.texture_id ].data );

		RasterizerVertex traingle_vertices[3];
		traingle_vertices[0]= verties_projected[0];
		for( unsigned int i= 0u; i < polygon_vertex_count - 2u; i++ )
		{
			traingle_vertices[1]= verties_projected[ i + 1u ];
			traingle_vertices[2]= verties_projected[ i + 2u ];
			rasterizer_.DrawTexturedTrianglePerLineCorrected( traingle_vertices );
		}
	}
}

void MapDrawerSoft::DrawModel(
	const ModelsGroup& models_group,
	const std::vector<Model>& models_group_models,
	const unsigned int model_id,
	const unsigned int animation_frame,
	const ViewClipPlanes& view_clip_planes,
	const m_Vec3& position,
	const m_Mat4& rotation_matrix,
	const m_Mat4& view_matrix )
{
	unsigned int active_clip_planes_mask= 0u;

	const m_BBox3& bbox= models_group_models[ model_id ].animations_bboxes[ animation_frame ];

	m_Mat4 translate_mat, bbox_mat;
	translate_mat.Translate( position );
	bbox_mat= rotation_matrix * translate_mat;

	// Clip-planes bounding box test
	for( const m_Plane3& clip_plane : view_clip_planes )
	{
		unsigned int vertices_inside= 0u;
		for( unsigned int z= 0u; z < 2u; z++ )
		for( unsigned int y= 0u; y < 2u; y++ )
		for( unsigned int x= 0u; x < 2u; x++ )
		{
			const m_Vec3 point(
				x == 0 ? bbox.min.x : bbox.max.x,
				y == 0 ? bbox.min.y : bbox.max.y,
				z == 0 ? bbox.min.z : bbox.max.z );

			if( clip_plane.IsPointAheadPlane( point * bbox_mat ) )
				vertices_inside++;
		}

		if( vertices_inside == 0u )
			return; // Discard model - it is fully outside view

		if( vertices_inside != 8u )
			active_clip_planes_mask|= 1u << ( &clip_plane - &view_clip_planes[0] );
	} // For clip planes

	// Transform clip planes into model space.
	ViewClipPlanes clip_planes_transformed;
	unsigned int clip_planes_transformed_count= 0u;

	m_Mat4 inv_rotation_mat= rotation_matrix;
	inv_rotation_mat.Transpose(); // For rotation matrix transpose is euqivalent for inverse.

	for( unsigned int i= 0u; i < view_clip_planes.size(); i++ )
	{
		if( ( active_clip_planes_mask & ( 1 << i ) ) == 0u )
			continue;

		const m_Plane3& in_plane= view_clip_planes[i];
		m_Plane3& out_plane= clip_planes_transformed[ clip_planes_transformed_count ];
		clip_planes_transformed_count++;

		out_plane.normal= in_plane.normal * inv_rotation_mat;
		out_plane.dist= in_plane.dist + in_plane.normal * position;
	}

	// Calculate final matrix
	const m_Mat4 final_mat= rotation_matrix * translate_mat * view_matrix;

	const Model& model= models_group_models[ model_id ];
	const unsigned int first_animation_vertex= model.animations_vertices.size() / model.frame_count * animation_frame;

	const ModelsGroup::ModelEntry& model_entry= models_group.models[ model_id ];
	rasterizer_.SetTexture(
		model_entry.texture_size[0], model_entry.texture_size[1],
		models_group.textures_data.data() + model_entry.texture_data_offset );

	// TODO - make other branch, if clip_planes_transformed_count == 0
	// Transform animation vertices, then rasterize trianglez directly, without clipping.

	// TODO - use original QUADS from .3o/.car models.

	for( unsigned int t= 0u; t < model.regular_triangles_indeces.size(); t+= 3u )
	{
		for( unsigned int tv= 0u; tv < 3u; tv++ )
		{
			const Model::Vertex& vertex= model.vertices[ model.regular_triangles_indeces[t + tv] ];
			const Model::AnimationVertex& animation_vertex= model.animations_vertices[ first_animation_vertex + vertex.vertex_id ];

			clipped_vertices_[tv].pos= m_Vec3( float(animation_vertex.pos[0]), float(animation_vertex.pos[1]), float(animation_vertex.pos[2]) ) / 2048.0f;
			clipped_vertices_[tv].tc.x= vertex.tex_coord[0] * float(model.texture_size[0]) * 65536.0f;
			clipped_vertices_[tv].tc.y= vertex.tex_coord[1] * float(model.texture_size[1]) * 65536.0f;
		}
		clipped_vertices_[0].next= &clipped_vertices_[1];
		clipped_vertices_[1].next= &clipped_vertices_[2];
		clipped_vertices_[2].next= &clipped_vertices_[0];
		fisrt_clipped_vertex_= &clipped_vertices_[0];
		next_new_clipped_vertex_= 3u;

		unsigned int polygon_vertex_count= 3u;
		for( unsigned int p= 0u; p < clip_planes_transformed_count; p++ )
		{
			polygon_vertex_count= ClipPolygon( clip_planes_transformed[p], polygon_vertex_count );
			PC_ASSERT( polygon_vertex_count == 0u || polygon_vertex_count >= 3u );
			if( polygon_vertex_count == 0u )
				break;
		}
		if( polygon_vertex_count == 0u )
			continue;

		RasterizerVertex verties_projected[ c_max_clip_vertices_ ];
		ClippedVertex* v= fisrt_clipped_vertex_;
		for( unsigned int i= 0u; i < polygon_vertex_count; i++, v= v->next )
		{
			m_Vec3 vertex_projected= v->pos * final_mat;
			const float w= v->pos.x * final_mat.value[3] + v->pos.y * final_mat.value[7] + v->pos.z * final_mat.value[11] + final_mat.value[15];

			vertex_projected/= w;
			vertex_projected.z= w;

			vertex_projected.x= ( vertex_projected.x + 1.0f ) * screen_transform_x_;
			vertex_projected.y= ( vertex_projected.y + 1.0f ) * screen_transform_y_;

			RasterizerVertex& out_v= verties_projected[ i ];
			out_v.x= fixed16_t( vertex_projected.x * 65536.0f );
			out_v.y= fixed16_t( vertex_projected.y * 65536.0f );
			out_v.u= fixed16_t( v->tc.x );
			out_v.v= fixed16_t( v->tc.y );
			out_v.z= fixed16_t( w * 65536.0f );
		}

		RasterizerVertex traingle_vertices[3];
		traingle_vertices[0]= verties_projected[0];
		for( unsigned int i= 0u; i < polygon_vertex_count - 2u; i++ )
		{
			traingle_vertices[1]= verties_projected[ i + 1u ];
			traingle_vertices[2]= verties_projected[ i + 2u ];
			rasterizer_.DrawAffineTexturedTriangle( traingle_vertices );
		}
	} // for model triangles
}

unsigned int MapDrawerSoft::ClipPolygon(
	const m_Plane3& clip_plane,
	unsigned int vertex_count )
{
	PC_ASSERT( vertex_count >= 3u );

	unsigned int i;
	bool positions[ c_max_clip_vertices_ ];

	unsigned int vertices_ahead= 0u, vertices_behind= 0u;
	ClippedVertex* last_vertex= nullptr;

	i= 0u;
	for( ClippedVertex* v= fisrt_clipped_vertex_; i < vertex_count; v= v->next, i++ )
	{
		positions[i]= clip_plane.IsPointAheadPlane( v->pos );
		if( positions[i] )
			vertices_ahead++;
		else
			vertices_behind++;

		if( i == vertex_count - 1u )
			last_vertex= v;
	}

	if( vertices_ahead == 0u )
		return 0u;
	if( vertices_ahead == vertex_count )
		return vertex_count;

	ClippedVertex* last_clipped_vertex= nullptr;
	ClippedVertex* last_unclipped_vertex= nullptr;

	if(  positions[ vertex_count - 1u ] && !positions[0] )
		last_unclipped_vertex= last_vertex;
	if( !positions[ vertex_count - 1u ] &&  positions[0] )
		last_clipped_vertex= last_vertex;

	i= 1u;
	for( ClippedVertex* prev_v= fisrt_clipped_vertex_; i < vertex_count; prev_v= prev_v->next, i++ )
	{
		if(  positions[ i - 1u ] && !positions[i] )
			last_unclipped_vertex= prev_v;
		if( !positions[ i - 1u ] &&  positions[i] )
			last_clipped_vertex= prev_v;
	}

	if( last_clipped_vertex == nullptr || last_unclipped_vertex == nullptr )
	{
		PC_ASSERT( false );
		return 0u;
	}

	PC_ASSERT( next_new_clipped_vertex_ + 2u <= c_max_clip_vertices_ );
	ClippedVertex* new_v0= &clipped_vertices_[ next_new_clipped_vertex_ ];
	ClippedVertex* new_v1= &clipped_vertices_[ next_new_clipped_vertex_ + 1u ];
	next_new_clipped_vertex_+= 2u;

	{
		const float dist0= +clip_plane.GetSignedDistance( last_unclipped_vertex->pos );
		const float dist1= -clip_plane.GetSignedDistance( last_unclipped_vertex->next->pos );
		PC_ASSERT( dist0 >= 0.0f && dist1 >= 0.0f );
		const float dist_inv_sum= 1.0f / ( dist0 + dist1 );
		const float k0= dist0 * dist_inv_sum;
		const float k1= dist1 * dist_inv_sum;
		new_v0->pos= last_unclipped_vertex->pos * k1 + last_unclipped_vertex->next->pos * k0;
		new_v0->tc = last_unclipped_vertex->tc  * k1 + last_unclipped_vertex->next->tc  * k0;
	}
	{
		const float dist0= -clip_plane.GetSignedDistance( last_clipped_vertex->pos );
		const float dist1= +clip_plane.GetSignedDistance( last_clipped_vertex->next->pos );
		PC_ASSERT( dist0 >= 0.0f && dist1 >= 0.0f );
		const float dist_inv_sum= 1.0f / ( dist0 + dist1 );
		const float k0= dist0 * dist_inv_sum;
		const float k1= dist1 * dist_inv_sum;
		new_v1->pos= last_clipped_vertex->pos * k1 + last_clipped_vertex->next->pos * k0;
		new_v1->tc=  last_clipped_vertex->tc  * k1 + last_clipped_vertex->next->tc  * k0;
	}

	last_unclipped_vertex->next= new_v0;
	new_v0->next= new_v1;
	new_v1->next= last_clipped_vertex->next;

	fisrt_clipped_vertex_= new_v0;

	return vertex_count - vertices_behind + 2u;
}

bool MapDrawerSoft::BBoxIsOutsideView(
	const ViewClipPlanes& clip_planes,
	const m_BBox3& bbox,
	const m_Mat4& bbox_mat )
{
	for( const m_Plane3& plane : clip_planes )
	{
		bool is_inside= false;
		for( unsigned int z= 0u; z < 2u; z++ )
		for( unsigned int y= 0u; y < 2u; y++ )
		for( unsigned int x= 0u; x < 2u; x++ )
		{
			const m_Vec3 point(
				x == 0 ? bbox.min.x : bbox.max.x,
				y == 0 ? bbox.min.y : bbox.max.y,
				z == 0 ? bbox.min.z : bbox.max.z );

			if( plane.IsPointAheadPlane( point * bbox_mat ) )
			{
				is_inside= true;
				goto box_vertices_check_end;
			}
		}
		box_vertices_check_end:

		if( !is_inside ) // bbox is behind of one of clip planes
			return true;
	}

	return false;
}

} // PanzerChasm
