#include "../assert.hpp"
#include "../game_constants.hpp"
#include "../map_loader.hpp"
#include "../math_utils.hpp"

#include "map_drawer_soft.hpp"

namespace PanzerChasm
{

MapDrawerSoft::MapDrawerSoft(
	Settings& settings,
	const GameResourcesConstPtr& game_resources,
	const RenderingContextSoft& rendering_context )
	: game_resources_( game_resources )
	, rendering_context_( rendering_context )
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
		return;

	LoadModelsGroup( map_data->models, map_models_ );
	LoadFloorsTextures( *map_data );
	LoadFloorsAndCeilings( *map_data );
}

void MapDrawerSoft::Draw(
	const MapState& map_state,
	const m_Mat4& view_rotation_and_projection_matrix,
	const m_Vec3& camera_position,
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

	DrawFloorsAndCeilings( cam_mat );

	for( const MapState::StaticModel& static_model : map_state.GetStaticModels() )
	{
		if( static_model.model_id >= current_map_data_->models_description.size() )
			continue;

		m_Mat4 rotate_mat, translate_mat, view_mat;
		rotate_mat.RotateZ( static_model.angle );
		translate_mat.Translate( static_model.pos );

		view_mat= rotate_mat * translate_mat * cam_mat;

		DrawModel( view_mat, map_models_, current_map_data_->models, static_model.model_id, static_model.animation_frame );
	}

	for( const MapState::Item& item : map_state.GetItems() )
	{
		if( item.item_id >= game_resources_->items_models.size() )
			continue;

		m_Mat4 rotate_mat, translate_mat, view_mat;
		rotate_mat.RotateZ( item.angle );
		translate_mat.Translate( item.pos );

		view_mat= rotate_mat * translate_mat * cam_mat;

		DrawModel( view_mat, items_models_, game_resources_->items_models, item.item_id, item.animation_frame );
	}

	for( const MapState::DynamicItemsContainer::value_type& dynamic_item_value : map_state.GetDynamicItems() )
	{
		const MapState::DynamicItem& item= dynamic_item_value.second;
		if( item.item_type_id >= game_resources_->items_models.size() )
			continue;

		m_Mat4 rotate_mat, translate_mat, view_mat;
		rotate_mat.RotateZ( item.angle );
		translate_mat.Translate( item.pos );

		view_mat= rotate_mat * translate_mat * cam_mat;

		DrawModel( view_mat, items_models_, game_resources_->items_models, item.item_type_id, item.frame );
	}

	for( const MapState::RocketsContainer::value_type& rocket_value : map_state.GetRockets() )
	{
		const MapState::Rocket& rocket= rocket_value.second;
		if( rocket.rocket_id >= game_resources_->rockets_models.size() )
			continue;

		m_Mat4 rotate_max_x, rotate_mat_z, translate_mat, view_mat;
		rotate_max_x.RotateX( rocket.angle[1] );
		rotate_mat_z.RotateZ( rocket.angle[0] - Constants::half_pi );
		translate_mat.Translate( rocket.pos );

		view_mat= rotate_max_x * rotate_mat_z * translate_mat * cam_mat;

		DrawModel( view_mat, rockets_models_, game_resources_->rockets_models, rocket.rocket_id, rocket.frame );
	}

	for( const MapState::MonstersContainer::value_type& monster_value : map_state.GetMonsters() )
	{
		const MapState::Monster& monster= monster_value.second;
		if( monster.monster_id >= game_resources_->monsters_models.size() )
			continue;

		if( monster_value.first == player_monster_id )
			continue;

		m_Mat4 rotate_mat, translate_mat, view_mat;
		rotate_mat.RotateZ( monster.angle + Constants::half_pi );
		translate_mat.Translate( monster.pos );

		view_mat= rotate_mat * translate_mat * cam_mat;

		const unsigned int frame=
			game_resources_->monsters_models[ monster.monster_id ].animations[ monster.animation ].first_frame +
			monster.animation_frame;

		DrawModel( view_mat, monsters_models_, game_resources_->monsters_models, monster.monster_id, frame );
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

void MapDrawerSoft::DrawFloorsAndCeilings( const m_Mat4& matrix )
{
	const float viewport_size_x= float(rendering_context_.viewport_size.Width ());
	const float viewport_size_y= float(rendering_context_.viewport_size.Height());
	const float screen_transform_x= viewport_size_x * 0.5f;
	const float screen_transform_y= viewport_size_y * 0.5f;

	for( unsigned int i= 0u; i < map_floors_and_ceilings_.size(); i++ )
	{
		const FloorCeilingCell& cell= map_floors_and_ceilings_[i];
		const float z= i < first_ceiling_ ? 0.0f : GameConstants::walls_height;

		RasterizerVertexTextured vertices_fixed[4];
		bool clipped= false;
		for( unsigned int y= 0u; y < 2u; y++ )
		for( unsigned int x= 0u; x < 2u; x++ )
		{
			const m_Vec3 vertex_pos( float(cell.xy[0] + x), float(cell.xy[1] + y), z );
			m_Vec3 vertex_projected= vertex_pos * matrix;
			const float w= vertex_pos.x * matrix.value[3] + vertex_pos.y * matrix.value[7] + vertex_pos.z * matrix.value[11] + matrix.value[15];
			if( w <= 0.25f )
			{
				clipped= true;
				break;
			}

			vertex_projected/= w;
			vertex_projected.z= w;

			vertex_projected.x= ( vertex_projected.x + 1.0f ) * screen_transform_x;
			vertex_projected.y= ( vertex_projected.y + 1.0f ) * screen_transform_y;

			if( vertex_projected.x < 0.0f || vertex_projected.x > viewport_size_x ||
				vertex_projected.y < 0.0f || vertex_projected.y > viewport_size_y )
			{
				clipped= true;
				goto after_clip;
			}

			RasterizerVertexTextured& out_v= vertices_fixed[ x + y * 2u ];
			out_v.x= fixed16_t( vertex_projected.x * 65536.0f );
			out_v.y= fixed16_t( vertex_projected.y * 65536.0f );
			out_v.u= fixed16_t( x * ( MapData::c_floor_texture_size << 16u ) );
			out_v.v= fixed16_t( y * ( MapData::c_floor_texture_size << 16u ) );
			out_v.z= fixed16_t( w * 65536.0f );
		}
		after_clip:
		if( clipped ) continue;

		rasterizer_.SetTexture(
			MapData::c_floor_texture_size, MapData::c_floor_texture_size,
			floor_textures_[ cell.texture_id ].data );

		rasterizer_.DrawAffineTexturedTriangle( vertices_fixed );
		rasterizer_.DrawAffineTexturedTriangle( vertices_fixed + 1u );
	}
}

void MapDrawerSoft::DrawModel(
	const m_Mat4& matrix,
	const ModelsGroup& models_group,
	const std::vector<Model>& models_group_models,
	unsigned int model_id,
	unsigned int animation_frame )
{
	const float viewport_size_x= float(rendering_context_.viewport_size.Width ());
	const float viewport_size_y= float(rendering_context_.viewport_size.Height());
	const float screen_transform_x= viewport_size_x * 0.5f;
	const float screen_transform_y= viewport_size_y * 0.5f;

	const Model& model= models_group_models[ model_id ];
	const unsigned int first_animation_vertex= model.animations_vertices.size() / model.frame_count * animation_frame;

	const ModelsGroup::ModelEntry& model_entry= models_group.models[ model_id ];
	rasterizer_.SetTexture(
		model_entry.texture_size[0], model_entry.texture_size[1],
		models_group.textures_data.data() + model_entry.texture_data_offset );

	for( unsigned int t= 0u; t < model.regular_triangles_indeces.size(); t+= 3u )
	{
		RasterizerVertexTextured vertices_fixed[3];
		bool clipped= false;
		for( unsigned int tv= 0u; tv < 3u; tv++ )
		{
			const Model::Vertex& vertex= model.vertices[ model.regular_triangles_indeces[t + tv] ];
			const Model::AnimationVertex& animation_vertex= model.animations_vertices[ first_animation_vertex + vertex.vertex_id ];

			const m_Vec3 vertex_pos= m_Vec3( float(animation_vertex.pos[0]), float(animation_vertex.pos[1]), float(animation_vertex.pos[2]) ) / 2048.0f;
			m_Vec3 vertex_projected= vertex_pos * matrix;
			const float w= vertex_pos.x * matrix.value[3] + vertex_pos.y * matrix.value[7] + vertex_pos.z * matrix.value[11] + matrix.value[15];
			if( w <= 0.25f )
			{
				clipped= true;
				break;
			}

			vertex_projected/= w;
			vertex_projected.z= w;

			vertex_projected.x= ( vertex_projected.x + 1.0f ) * screen_transform_x;
			vertex_projected.y= ( vertex_projected.y + 1.0f ) * screen_transform_y;

			if( vertex_projected.x < 0.0f || vertex_projected.x > viewport_size_x ||
				vertex_projected.y < 0.0f || vertex_projected.y > viewport_size_y )
			{
				clipped= true;
				break;
			}

			vertices_fixed[tv].x= fixed16_t( vertex_projected.x * 65536.0f );
			vertices_fixed[tv].y= fixed16_t( vertex_projected.y * 65536.0f );
			vertices_fixed[tv].u= fixed16_t( vertex.tex_coord[0] * float(model.texture_size[0]) * 65536.0f );
			vertices_fixed[tv].v= fixed16_t( vertex.tex_coord[1] * float(model.texture_size[1]) * 65536.0f );
			vertices_fixed[tv].z= fixed16_t( w * 65536.0f );
		}
		if( clipped ) continue;

		// Back side culling.
		// TODO - make it befor frustrum culling.
		fixed16_t vecs[2][2];
		vecs[0][0]= vertices_fixed[1].x - vertices_fixed[0].x;
		vecs[0][1]= vertices_fixed[1].y - vertices_fixed[0].y;
		vecs[1][0]= vertices_fixed[2].x - vertices_fixed[1].x;
		vecs[1][1]= vertices_fixed[2].y - vertices_fixed[1].y;
		if( Fixed16Mul( vecs[0][0], vecs[1][1] ) > Fixed16Mul( vecs[1][0], vecs[0][1] ) )
			continue;

		rasterizer_.DrawAffineTexturedTriangle( vertices_fixed );
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
				texture_number == MapData::c_sky_floor_texture_id )
				continue;

			map_floors_and_ceilings_.emplace_back();
			FloorCeilingCell& cell= map_floors_and_ceilings_.back();
			cell.xy[0]= x;
			cell.xy[1]= y;
			cell.texture_id= texture_number;
		}
	}
}

} // PanzerChasm
