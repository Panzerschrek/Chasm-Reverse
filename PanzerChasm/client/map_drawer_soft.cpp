#include <cstring>

#include "../assert.hpp"
#include "../game_constants.hpp"
#include "../log.hpp"
#include "../map_loader.hpp"
#include "../math_utils.hpp"
#include "../settings.hpp"
#include "../shared_settings_keys.hpp"
#include "map_drawers_common.hpp"
#include "software_renderer/map_bsp_tree.hpp"
#include "software_renderer/map_bsp_tree.inl"
#include "software_renderer/rasterizer.inl"

#include "map_drawer_soft.hpp"

namespace PanzerChasm
{

static void BuildMip(
	const uint32_t* const in_data, const unsigned int in_size_x, const unsigned int in_size_y,
	uint32_t* const out_data )
{
	const unsigned int out_size_x= in_size_x >> 1u;
	const unsigned int out_size_y= in_size_y >> 1u;
	for( unsigned int y= 0u; y < out_size_y; y++ )
	for( unsigned int x= 0u; x < out_size_x; x++ )
	{
		uint32_t pixels[4];
		const unsigned int src_x= x << 1u;
		const unsigned int src_y= y << 1u;
		pixels[0]= in_data[ src_x +        src_y        * in_size_x ];
		pixels[1]= in_data[ src_x + 1u +   src_y        * in_size_x ];
		pixels[2]= in_data[ src_x + 1u + ( src_y + 1u ) * in_size_x ];
		pixels[3]= in_data[ src_x +      ( src_y + 1u ) * in_size_x ];

		unsigned char components[4];
		for( unsigned int c= 0u; c < 4u; c++ )
		{
			components[c]= (
				reinterpret_cast<const unsigned char*>(&pixels[0])[c] +
				reinterpret_cast<const unsigned char*>(&pixels[1])[c] +
				reinterpret_cast<const unsigned char*>(&pixels[2])[c] +
				reinterpret_cast<const unsigned char*>(&pixels[3])[c] ) >> 2u;
		}
		std::memcpy( &out_data[ x + y * out_size_x ], components, sizeof(uint32_t) );
	}
}

static void BuildMipAlphaCorrected(
	const uint32_t* const in_data, const unsigned int in_size_x, const unsigned int in_size_y,
	uint32_t* const out_data )
{
	const unsigned int out_size_x= in_size_x >> 1u;
	const unsigned int out_size_y= in_size_y >> 1u;
	for( unsigned int y= 0u; y < out_size_y; y++ )
	for( unsigned int x= 0u; x < out_size_x; x++ )
	{
		uint32_t pixels[4];
		const unsigned int src_x= x << 1u;
		const unsigned int src_y= y << 1u;
		pixels[0]= in_data[ src_x +        src_y        * in_size_x ];
		pixels[1]= in_data[ src_x + 1u +   src_y        * in_size_x ];
		pixels[2]= in_data[ src_x + 1u + ( src_y + 1u ) * in_size_x ];
		pixels[3]= in_data[ src_x +      ( src_y + 1u ) * in_size_x ];

		// Calculate avg color, except for pixels with full alpha (=0)
		unsigned int components_sum[4]= { 0u, 0u, 0u, 0u };
		unsigned int nonalpha_pixels= 0u;
		for( unsigned int p= 0u; p < 4u; p++ )
		{
			if( (pixels[p] & Rasterizer::c_alpha_mask) != 0u )
			{
				nonalpha_pixels++;
				for( unsigned int c= 0u; c < 3u; c++ )
					components_sum[c]+= reinterpret_cast<const unsigned char*>(&pixels[p])[c];
			} // else - full alpha
			components_sum[3]+= reinterpret_cast<const unsigned char*>(&pixels[p])[3];
		}
		unsigned char components[4];
		if( nonalpha_pixels > 0u )
		{
			for( unsigned int c= 0u; c < 3u; c++ )
				components[c]= components_sum[c] / nonalpha_pixels;
		}
		else
		{
			for( unsigned int c= 0u; c < 3u; c++ )
				components[c]= 0u;
		}
		components[3]= components_sum[3] >> 2u;

		std::memcpy( &out_data[ x + y * out_size_x ], components, sizeof(uint32_t) );
	}
}

static void MakeBinaryAlpha( uint32_t* const pixels, const unsigned int pixel_count )
{
	for( unsigned int i= 0u; i < pixel_count; i++ )
	{
		// TODO - calibrate alpha edge.
		if( ( pixels[i] & Rasterizer::c_alpha_mask ) < Rasterizer::c_alpha_mask / 3u )
			pixels[i]&= ~Rasterizer::c_alpha_mask;
	}
}

static fixed16_t ScaleLightmapLight( const unsigned char lightmap_value )
{
	// Overbright constant must be equal to same constant in shader. See shaders/constants.glsl.
	static constexpr float c_overbright= 1.3f;
	static constexpr fixed16_t scale= static_cast<fixed16_t>( ( c_overbright * 65536.0f ) / 255.0f );

	return lightmap_value * scale;
}

MapDrawerSoft::MapDrawerSoft(
	Settings& settings,
	const GameResourcesConstPtr& game_resources,
	const RenderingContextSoft& rendering_context )
	: settings_(settings)
	, game_resources_( game_resources )
	, rendering_context_( rendering_context )
	, screen_transform_x_( 0.5f * float( rendering_context_.viewport_size.Width () ) )
	, screen_transform_y_( 0.5f * float( rendering_context_.viewport_size.Height() ) )
	, rasterizer_(
		rendering_context.viewport_size.Width(), rendering_context.viewport_size.Height(),
		rendering_context.row_pixels, rendering_context.window_surface_data )
	, surfaces_cache_( rendering_context_.viewport_size )
{
	PC_ASSERT( game_resources_ != nullptr );

	sky_texture_.file_name[0]= '\0';

	LoadModelsGroup( game_resources_->items_models, items_models_ );
	LoadModelsGroup( game_resources_->rockets_models, rockets_models_ );
	LoadModelsGroup( game_resources_->gibs_models, gibs_models_ );
	LoadModelsGroup( game_resources_->weapons_models, weapons_models_ );
	LoadModelsGroup( game_resources_->monsters_models, monsters_models_ );

	// Load effects sprites.

	const PaletteTransformed& palette= *rendering_context_.palette_transformed;

	sprite_effects_textures_.resize( game_resources_->effects_sprites.size() );
	for( unsigned int i= 0u; i < sprite_effects_textures_.size(); i++ )
	{
		const ObjSprite& in_sprite= game_resources_->effects_sprites[i];
		SpriteTexture& out_sprite_texture= sprite_effects_textures_[i];

		out_sprite_texture.size[0]= in_sprite.size[0];
		out_sprite_texture.size[1]= in_sprite.size[1];
		out_sprite_texture.size[2]= in_sprite.frame_count;

		const unsigned int pixel_count= in_sprite.size[0] * in_sprite.size[1] * in_sprite.frame_count;
		out_sprite_texture.data.resize( pixel_count );
		for( unsigned int j= 0u; j < pixel_count; j++ )
			out_sprite_texture.data[j]= palette[in_sprite.data[j]];
	}

	bmp_objects_sprites_.resize( game_resources_->bmp_objects_sprites.size() );
	for( unsigned int i= 0u; i < bmp_objects_sprites_.size(); i++ )
	{
		const ObjSprite& in_sprite= game_resources_->bmp_objects_sprites[i];
		SpriteTexture& out_sprite_texture= bmp_objects_sprites_[i];

		out_sprite_texture.size[0]= in_sprite.size[0];
		out_sprite_texture.size[1]= in_sprite.size[1];
		out_sprite_texture.size[2]= in_sprite.frame_count;

		const unsigned int pixel_count= in_sprite.size[0] * in_sprite.size[1] * in_sprite.frame_count;
		out_sprite_texture.data.resize( pixel_count );
		for( unsigned int j= 0u; j < pixel_count; j++ )
			out_sprite_texture.data[j]= palette[in_sprite.data[j]];
	}
}

MapDrawerSoft::~MapDrawerSoft()
{}

void MapDrawerSoft::SetMap( const MapDataConstPtr& map_data )
{
	if( map_data == current_map_data_ )
		return;

	current_map_data_= map_data;
	if( map_data == nullptr )
		return; // TODO - if map is null - clear resources, etc.

	surfaces_cache_.Clear();

	map_bsp_tree_.reset( new MapBSPTree( map_data ) );

	LoadModelsGroup( map_data->models, map_models_ );
	LoadWallsTextures( *map_data );
	LoadFloorsTextures( *map_data );
	LoadWalls( *map_data );
	LoadFloorsAndCeilings( *map_data );

	// Sky
	if( std::strcmp( sky_texture_.file_name, current_map_data_->sky_texture_name ) != 0 )
	{
		std::strncpy( sky_texture_.file_name, current_map_data_->sky_texture_name, sizeof(sky_texture_.file_name) );

		char sky_texture_file_path[ MapData::c_max_file_path_size ];
		std::snprintf( sky_texture_file_path, sizeof(sky_texture_file_path), "COMMON/%s", current_map_data_->sky_texture_name );

		const Vfs::FileContent sky_texture_data= game_resources_->vfs->ReadFile( sky_texture_file_path );
		const CelTextureHeader& cel_header= *reinterpret_cast<const CelTextureHeader*>( sky_texture_data.data() );

		const unsigned int sky_pixel_count= cel_header.size[0] * cel_header.size[1];
		sky_texture_.data.resize( sky_pixel_count );

		const PaletteTransformed& palette= *rendering_context_.palette_transformed;
		const unsigned char* const src= sky_texture_data.data() + sizeof(CelTextureHeader);

		for( unsigned int i= 0u; i < sky_pixel_count; i++ )
			sky_texture_.data[i]= palette[src[i]];

		sky_texture_.size[0]= cel_header.size[0];
		sky_texture_.size[1]= cel_header.size[1];
	}
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
	rasterizer_.ClearOcclusionBuffer();

	m_Mat4 cam_shift_mat, cam_mat, screen_flip_mat;
	cam_shift_mat.Translate( -camera_position );
	screen_flip_mat.Scale( m_Vec3( 1.0f, -1.0f, 1.0f ) );
	cam_mat= cam_shift_mat * view_rotation_and_projection_matrix * screen_flip_mat;

	// Draw objects front to back with occlusion test.
	// Occlusion test uses walls, floors/ceilings, sky.
	DrawWalls( map_state, cam_mat, camera_position.xy(), view_clip_planes );
	DrawFloorsAndCeilings( cam_mat, view_clip_planes );
	DrawSky( cam_mat, camera_position, view_clip_planes );

	rasterizer_.BuildDepthBufferHierarchy();

	// Draw regular polygons of models, than transparent
	for( unsigned int t= 0u; t < 2u; t++ )
	{
		const bool transparent= t == 1u;

		for( const MapState::StaticModel& static_model : map_state.GetStaticModels() )
		{
			if( static_model.model_id >= current_map_data_->models_description.size() ||
				!static_model.visible )
				continue;

			m_Mat4 rotate_mat;
			rotate_mat.RotateZ( static_model.angle );

			DrawModel(
				map_models_, current_map_data_->models, static_model.model_id,
				static_model.animation_frame,
				view_clip_planes,
				static_model.pos, rotate_mat,
				cam_mat, camera_position,
				255u, transparent );
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
				item.pos, rotate_mat,
				cam_mat, camera_position,
				255u, transparent );
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
				item.pos, rotate_mat,
				cam_mat, camera_position,
				255u, transparent,
				item.fullbright );
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
				rocket.pos, rotate_max_x * rotate_mat_z,
				cam_mat, camera_position,
				255u, transparent,
				game_resources_->rockets_description[ rocket.rocket_id ].fullbright );
		}

		for( const MapState::Gib& gib : map_state.GetGibs() )
		{
			if( gib.gib_id >= gibs_models_.models.size() )
				continue;

			m_Mat4 rotate_max_x, rotate_mat_z;
			rotate_max_x.RotateX( gib.angle_x );
			rotate_mat_z.RotateZ( gib.angle_z );

			DrawModel(
				gibs_models_, game_resources_->gibs_models, gib.gib_id,
				0u,
				view_clip_planes,
				gib.pos, rotate_max_x * rotate_mat_z,
				cam_mat, camera_position,
				255u, transparent );
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
				monster.pos, rotate_mat,
				cam_mat, camera_position,
				monster.body_parts_mask, transparent,
				false, ~0u, monster.color );
		}

		for( const MapState::MonsterBodyPart& part : map_state.GetMonstersBodyParts() )
		{
			if( part.monster_type >= game_resources_->monsters_models.size() )
				continue;

			PC_ASSERT( part.body_part_id <= game_resources_->monsters_models[ part.monster_type ].submodels.size() );

			const Submodel& submodel= game_resources_->monsters_models[ part.monster_type ].submodels[ part.body_part_id ];
			const unsigned int frame= submodel.animations[ part.animation ].first_frame + part.animation_frame;

			m_Mat4 rotate_mat;
			rotate_mat.RotateZ( part.angle + Constants::half_pi );

			DrawModel(
				monsters_models_, game_resources_->monsters_models, part.monster_type,
				frame,
				view_clip_planes,
				part.pos, rotate_mat,
				cam_mat, camera_position,
				255u, transparent, false,
				part.body_part_id );
		}
	}

	// Shadows.
	if( settings_.GetOrSetBool( SettingsKeys::shadows, true ) )
	{
		for( const MapState::StaticModel& static_model : map_state.GetStaticModels() )
		{
			if( static_model.model_id >= current_map_data_->models_description.size() ||
				!static_model.visible )
				continue;

			const MapData::ModelDescription& description= current_map_data_->models_description[ static_model.model_id ];
			if( !description.cast_shadow )
				continue;

			m_Vec3 light_pos;
			if( !GetNearestLightSourcePos( static_model.pos, *current_map_data_, map_state, false, light_pos ) )
				continue;

			m_Mat4 rotate_mat;
			rotate_mat.RotateZ( static_model.angle );

			DrawModelShadow(
				current_map_data_->models[ static_model.model_id ],
				static_model.animation_frame,
				view_clip_planes,
				static_model.pos, rotate_mat,
				cam_mat, camera_position, light_pos,
				255u );
		}

		for( const MapState::Item& item : map_state.GetItems() )
		{
			if( item.item_id >= game_resources_->items_models.size() ||
				item.picked_up )
				continue;

			const GameResources::ItemDescription& description= game_resources_->items_description[ item.item_id ];
			if( !description.cast_shadow )
				continue;

			m_Vec3 light_pos;
			if( !GetNearestLightSourcePos( item.pos, *current_map_data_, map_state, true, light_pos ) )
				continue;

			m_Mat4 rotate_mat;
			rotate_mat.RotateZ( item.angle );

			DrawModelShadow(
				game_resources_->items_models[ item.item_id ],
				item.animation_frame,
				view_clip_planes,
				item.pos, rotate_mat,
				cam_mat, camera_position, light_pos,
				255u );
		}

		for( const MapState::MonstersContainer::value_type& monster_value : map_state.GetMonsters() )
		{
			const MapState::Monster& monster= monster_value.second;
			if( monster.monster_id >= game_resources_->monsters_models.size() )
				continue;
			if( monster_value.first == player_monster_id )
				continue;
			if( monster.is_fully_dead )
				continue;

			m_Vec3 light_pos;
			if( !GetNearestLightSourcePos( monster.pos, *current_map_data_, map_state, true, light_pos ) )
				continue;

			const unsigned int frame=
				game_resources_->monsters_models[ monster.monster_id ].animations[ monster.animation ].first_frame +
				monster.animation_frame;

			m_Mat4 rotate_mat;
			rotate_mat.RotateZ( monster.angle + Constants::half_pi );

			DrawModelShadow(
				game_resources_->monsters_models[ monster.monster_id ],
				frame,
				view_clip_planes,
				monster.pos, rotate_mat,
				cam_mat, camera_position, light_pos,
				monster.body_parts_mask );
		}
	} // if shadows

	// Transparent objects.

	DrawEffectsSprites( map_state, cam_mat, camera_position, view_clip_planes );
	DrawBMPObjectsSprites( map_state, cam_mat, camera_position, view_clip_planes );

	// Fullscreen blend.
	m_Vec3 blend_color;
	float blend_alpha;
	map_state.GetFullscreenBlend( blend_color, blend_alpha );
	if( blend_alpha > 0.001f )
	{
		unsigned char blend_color_i[3];
		unsigned int blend_alpha_i, one_minus_blend_alpha_i;
		for( unsigned int j= 0u; j < 3u; j++ )
		{
			int c= static_cast<int>( std::round( 255.0f * blend_color.ToArr()[j] ) );
			if( c < 0 ) c= 0;
			if( c > 255 ) c= 255;
			blend_color_i[ rendering_context_.color_indeces_rgba[j] ]= c;
		}

		blend_alpha_i= std::max( 0u, std::min( 65536u, static_cast<unsigned int>( std::round( blend_alpha * 65536.0f ) ) ) );
		one_minus_blend_alpha_i= 65536u - blend_alpha_i;

		uint32_t* const dst_pixels= rendering_context_.window_surface_data;
		const unsigned int pixel_count= rendering_context_.row_pixels * rendering_context_.viewport_size.Height();

		for( unsigned int i= 0u; i < pixel_count; i++ )
		{
			const uint32_t pixel_value= dst_pixels[i];
			unsigned char color[4];
			for( unsigned int j= 0u; j < 3u; j++ )
				color[j]= (
					reinterpret_cast<const unsigned char*>(&pixel_value)[j] * one_minus_blend_alpha_i +
					blend_color_i[j] * blend_alpha_i ) >> 16u;

			std::memcpy( &dst_pixels[i], color, sizeof(uint32_t) );
		}
	}

	if( settings_.GetOrSetBool( "r_debug_draw_depth_hierarchy", false ) )
		rasterizer_.DebugDrawDepthHierarchy( static_cast<unsigned int>(map_state.GetSpritesFrame()) / 16u );
	if( settings_.GetOrSetBool( "r_debug_draw_occlusion_buffer", false ) )
		rasterizer_.DebugDrawOcclusionBuffer( static_cast<unsigned int>(map_state.GetSpritesFrame()) / 32u );
}

void MapDrawerSoft::DrawWeapon(
	const WeaponState& weapon_state,
	const m_Mat4& projection_matrix,
	const m_Vec3& camera_position,
	const float x_angle, const float z_angle )
{
	PC_UNUSED( x_angle );
	PC_UNUSED( z_angle );

	m_Mat4 shift_mat, screen_flip_mat;
	const m_Vec3 additional_shift=
		g_weapon_shift + g_weapon_change_shift * ( 1.0f - weapon_state.GetSwitchStage() );
	screen_flip_mat.Scale( m_Vec3( 1.0f, -1.0f, 1.0f ) );
	shift_mat.Translate( additional_shift );

	const m_Mat4 final_mat= shift_mat * projection_matrix * screen_flip_mat;
	const m_Vec3 cam_pos_model_space= -additional_shift;

	// Use only near clip plane for weapon clipping.
	m_Plane3 clip_plane_transformed;
	clip_plane_transformed.normal= m_Vec3( 0.0f, 1.0f, 0.0f );
	const float z_near= 1.5f / float( 1 << Rasterizer::c_max_inv_z_min_log2 );
	clip_plane_transformed.dist= -z_near + additional_shift * clip_plane_transformed.normal;

	const Model& model= game_resources_->weapons_models[ weapon_state.CurrentWeaponIndex() ];
	const unsigned int frame= model.animations[ weapon_state.CurrentAnimation() ].first_frame + weapon_state.CurrentAnimationFrame();

	const unsigned int first_animation_vertex= model.animations_vertices.size() / model.frame_count * frame;

	const ModelsGroup::ModelEntry& model_entry= weapons_models_.models[ weapon_state.CurrentWeaponIndex() ];
	rasterizer_.SetTexture(
		model_entry.texture_size[0], model_entry.texture_size[1],
		weapons_models_.textures_data.data() + model_entry.texture_data_offset );

	{ // Set light.
		fixed16_t light= g_fixed16_one;
		const unsigned int lightmap_x= static_cast<unsigned int>( camera_position.x * float(MapData::c_lightmap_scale) );
		const unsigned int lightmap_y= static_cast<unsigned int>( camera_position.y * float(MapData::c_lightmap_scale) );
		if( lightmap_x < MapData::c_lightmap_size && lightmap_y < MapData::c_lightmap_size )
				light= ScaleLightmapLight( current_map_data_->lightmap[ lightmap_x + lightmap_y * MapData::c_lightmap_size ] );

		rasterizer_.SetLight( light );
	}

	Rasterizer::TriangleDrawFunc draw_func, alpha_draw_func;
	draw_func=
		&Rasterizer::DrawTexturedTriangleSpanCorrected<
			Rasterizer::DepthTest::Yes, Rasterizer::DepthWrite::Yes,
			Rasterizer::AlphaTest::No,
			Rasterizer::OcclusionTest::No, Rasterizer::OcclusionWrite::No,
			Rasterizer::Lighting::Yes, Rasterizer::Blending::No, Rasterizer::DepthHack::Yes>;
	alpha_draw_func=
		&Rasterizer::DrawTexturedTriangleSpanCorrected<
			Rasterizer::DepthTest::Yes, Rasterizer::DepthWrite::Yes,
			Rasterizer::AlphaTest::Yes,
			Rasterizer::OcclusionTest::No, Rasterizer::OcclusionWrite::No,
			Rasterizer::Lighting::Yes, Rasterizer::Blending::No, Rasterizer::DepthHack::Yes>;

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
		const m_Vec3 v0= clipped_vertices_[1].pos - clipped_vertices_[0].pos;
		const m_Vec3 v1= clipped_vertices_[2].pos - clipped_vertices_[0].pos;
		const m_Vec3 vec_to_cam= cam_pos_model_space - clipped_vertices_[0].pos;
		if( mVec3Cross( v0, v1 ) * vec_to_cam < 0.0f )
			continue;

		clipped_vertices_[0].next= &clipped_vertices_[1];
		clipped_vertices_[1].next= &clipped_vertices_[2];
		clipped_vertices_[2].next= &clipped_vertices_[0];
		fisrt_clipped_vertex_= &clipped_vertices_[0];
		next_new_clipped_vertex_= 3u;

		unsigned int polygon_vertex_count= 3u;
		polygon_vertex_count= ClipPolygon( clip_plane_transformed, polygon_vertex_count );
		PC_ASSERT( polygon_vertex_count == 0u || polygon_vertex_count >= 3u );
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

		const bool triangle_needs_alpha_test= model.vertices[ model.regular_triangles_indeces[t] ].alpha_test_mask != 0u;
		const Rasterizer::TriangleDrawFunc triangle_func= triangle_needs_alpha_test ? alpha_draw_func : draw_func;

		RasterizerVertex traingle_vertices[3];
		traingle_vertices[0]= verties_projected[0];
		for( unsigned int i= 0u; i < polygon_vertex_count - 2u; i++ )
		{
			traingle_vertices[1]= verties_projected[ i + 1u ];
			traingle_vertices[2]= verties_projected[ i + 2u ];
			(rasterizer_.*triangle_func)( traingle_vertices );
		}
	} // for model triangles
}

void MapDrawerSoft::DrawMapRelatedModels(
	const MapRelatedModel* const models, const unsigned int model_count,
	const m_Mat4& view_rotation_and_projection_matrix,
	const m_Vec3& camera_position,
	const ViewClipPlanes& view_clip_planes )
{
	if( current_map_data_ == nullptr )
		return;

	rasterizer_.ClearDepthBuffer();

	m_Mat4 cam_shift_mat, cam_mat, screen_flip_mat;
	cam_shift_mat.Translate( -camera_position );
	screen_flip_mat.Scale( m_Vec3( 1.0f, -1.0f, 1.0f ) );
	cam_mat= cam_shift_mat * view_rotation_and_projection_matrix * screen_flip_mat;

	for( unsigned int t= 0u; t < 2u; t++ )
	{
		const bool transparent= t > 0u;
		for( unsigned int m= 0u; m < model_count; m++ )
		{
			const MapRelatedModel& model= models[m];

			if( model.model_id >= current_map_data_->models_description.size() )
				continue;

			m_Mat4 rotate_mat;
			rotate_mat.RotateZ( model.angle_z );

			DrawModel(
				map_models_, current_map_data_->models, model.model_id,
				model.frame,
				view_clip_planes,
				model.pos, rotate_mat,
				cam_mat, camera_position,
				255u, transparent );
		} // for models
	}
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
		{
			const unsigned char color_index= in_model.texture_data[t];
			uint32_t color= palette[ color_index ];
			if( color_index == 0u ) color&= ~Rasterizer::c_alpha_mask; // For models color #0 is transparent.
			out_group.textures_data[ texture_data_offset + t ]= color;
		}

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
		if( header.size[0] % ( g_max_wall_texture_width / 16u ) != 0u ||
			header.size[1] != g_wall_texture_height )
		{
			Log::Warning( "Invalid wall texture size: ", header.size[0], "x", header.size[1] );
			continue;
		}

		out_texture.size[0]= header.size[0];
		out_texture.size[1]= header.size[1];

		const unsigned int pixel_count= header.size[0] * header.size[1];
		const unsigned int storage_size= pixel_count + pixel_count / 4u + pixel_count / 16u + pixel_count / 64u;
		const unsigned char* const src= file_content.data() + sizeof(CelTextureHeader);

		out_texture.data.resize( storage_size );
		out_texture.mip0= out_texture.data.data();
		out_texture.mips[0]= out_texture.mip0 + pixel_count;
		out_texture.mips[1]= out_texture.mips[0] + pixel_count /  4u;
		out_texture.mips[2]= out_texture.mips[1] + pixel_count / 16u;

		for( unsigned int j= 0u; j < pixel_count; j++ )
			out_texture.mip0[j]= palette[ src[j] ];
		BuildMipAlphaCorrected( out_texture.mip0   , out_texture.size[0]     , out_texture.size[1]     , out_texture.mips[0] );
		BuildMipAlphaCorrected( out_texture.mips[0], out_texture.size[0] / 2u, out_texture.size[1] / 2u, out_texture.mips[1] );
		BuildMipAlphaCorrected( out_texture.mips[1], out_texture.size[0] / 4u, out_texture.size[1] / 4u, out_texture.mips[2] );
		MakeBinaryAlpha( out_texture.mips[0], pixel_count /  4u );
		MakeBinaryAlpha( out_texture.mips[1], pixel_count / 16u );
		MakeBinaryAlpha( out_texture.mips[2], pixel_count / 64u );

		// Calculate top and bottom alpha-rejected texture rows.
		out_texture.full_alpha_row[0]= 0u;
		out_texture.full_alpha_row[1]= g_wall_texture_height;

		bool is_only_alpha= false;
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
			if( is_full_alpha && y + 1u == header.size[1] )
				is_only_alpha= true;
		}
		if( is_only_alpha )
		{
			// Texture contains only alpha pixels - mark it, and do not draw walls with this texture.
			out_texture.full_alpha_row[0]= out_texture.full_alpha_row[1]= 0u;
			out_texture.has_alpha= false;
			continue;
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
			if( is_full_alpha && y + 1u == header.size[1] )
				out_texture.full_alpha_row[1]= 0u;
		}

		bool has_alpha= false;
		for( unsigned int y= out_texture.full_alpha_row[0]; y < out_texture.full_alpha_row[1]; y++ )
		{
			for( unsigned int x= 0u; x < header.size[0]; x++ )
			if( src[x + y * header.size[0] ] == 255u )
			{
				has_alpha= true;
				break;
			}

			if( has_alpha )
				break;
		}
		out_texture.has_alpha= has_alpha;
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

		BuildMip( floor_textures_[i].data, MapData::c_floor_texture_size     , MapData::c_floor_texture_size     , floor_textures_[i].mip1 );
		BuildMip( floor_textures_[i].mip1, MapData::c_floor_texture_size / 2u, MapData::c_floor_texture_size / 2u, floor_textures_[i].mip2 );
		BuildMip( floor_textures_[i].mip2, MapData::c_floor_texture_size / 4u, MapData::c_floor_texture_size / 4u, floor_textures_[i].mip3 );
	}
}

void MapDrawerSoft::LoadWalls( const MapData& map_data )
{
	static_walls_ .resize( map_data.static_walls .size() );
	dynamic_walls_.resize( map_data.dynamic_walls.size() );

	const auto setup_wall=
	[]( const MapData::Wall& in_wall, DrawWall& out_wall ) -> void
	{
		out_wall.surface_width= static_cast<unsigned int>( 128.0f * in_wall.vert_tex_coord[0] );
		PC_ASSERT( out_wall.surface_width == 64u || out_wall.surface_width == 128u );

		out_wall.texture_id= in_wall.texture_id;
		std::memcpy( out_wall.lightmap, in_wall.lightmap, 8u );

		for( SurfacesCache::Surface*& surf_ptr : out_wall.mips_surfaces )
			surf_ptr= nullptr;
	};

	for( unsigned int i= 0u; i < static_walls_ .size(); i++ )
		setup_wall( map_data.static_walls [i], static_walls_ [i] );

	for( unsigned int i= 0u; i < dynamic_walls_.size(); i++ )
		setup_wall( map_data.dynamic_walls[i], dynamic_walls_[i] );
}

MapDrawerSoft::TextureView MapDrawerSoft::GetPlayerTexture( const unsigned char color )
{
	// Should be done after monsters loading.
	PC_ASSERT( !monsters_models_.models.empty() );

	const unsigned char color_corrected= color % GameConstants::player_colors_count;

	// Default texture (unshifted).
	if( color_corrected == 0u )
	{
		TextureView result;
		const ModelsGroup::ModelEntry& model_entry= monsters_models_.models.front();
		result.size[0]= model_entry.texture_size[0];
		result.size[1]= model_entry.texture_size[1];
		result.data= monsters_models_.textures_data.data() + model_entry.texture_data_offset;
		return result;
	}

	if( player_textures_.size() <= color_corrected )
		player_textures_.resize( color_corrected + 1u );

	PlayerTexture& texture= player_textures_[ color_corrected ];
	if( texture.data.empty() )
	{
		// Generate new texture and save it.
		const Model& model= game_resources_->monsters_models.front();
		const unsigned int pixel_count= model.texture_data.size();

		// TODO - maybe cache buffers?
		std::vector<unsigned char> data_shifted( pixel_count );
		texture.data.resize( pixel_count );

		ColorShift(
			14 * 16u, 14 * 16u + 16u,
			GameConstants::player_colors_shifts[ color_corrected ],
			pixel_count,
			model.texture_data.data(),
			data_shifted.data() );

		const PaletteTransformed& palette= *rendering_context_.palette_transformed;
		for( unsigned int i= 0u; i < pixel_count; i++ )
			texture.data[i]= palette[ data_shifted[i] ];

		texture.size[0]= model.texture_size[0];
		texture.size[1]= model.texture_size[1];
	}

	TextureView result;
	result.size[0]= texture.size[0];
	result.size[1]= texture.size[1];
	result.data= texture.data.data();
	return result;
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

			for( SurfacesCache::Surface*& surf_ptr : cell.mips_surfaces )
				surf_ptr= nullptr;
		}
	}
}

template< bool is_dynamic_wall >
void MapDrawerSoft::DrawWallSegment(
	DrawWall& wall,
	const m_Vec2& vert_pos0, const m_Vec2& vert_pos1, const float z,
	const float tc_0, const float tc_1,
	const m_Mat4& matrix,
	const m_Vec2& camera_position_xy,
	const ViewClipPlanes& view_clip_planes )
{
	PC_ASSERT( z >= 0.0f );

	PC_ASSERT( wall.texture_id < MapData::c_max_walls_textures );
	const WallTexture& texture= wall_textures_[ wall.texture_id ];
	if( texture.size[0] == 0u || texture.size[1] == 0u )
		return;
	if( texture.full_alpha_row[0] == texture.full_alpha_row[1] )
		return;

	// Discard back faces.
	// TODO - know, what discard criteria was in original game.
	const bool is_back= mVec2Cross( camera_position_xy - vert_pos0, vert_pos1 - vert_pos0 ) > 0.0f;
	if( !is_dynamic_wall &&
		wall.texture_id < MapData::c_first_transparent_texture_id &&
		is_back )
		return;

	const float z_bottom_top[]=
	{
		GameConstants::walls_height - float(texture.full_alpha_row[0]) * ( GameConstants::walls_height / float(g_wall_texture_height) ),
		z + GameConstants::walls_height - float(texture.full_alpha_row[1]) * ( GameConstants::walls_height / float(g_wall_texture_height) )
	};
	const float tc_top= float( texture.full_alpha_row[0] << 16u ) + z * ( 65536.0f * float(g_wall_texture_height) / float(GameConstants::walls_height) );

	const float tc_scale= float( wall.surface_width << 16u );

	clipped_vertices_[0].pos= m_Vec3( vert_pos0, z_bottom_top[0] );
	clipped_vertices_[1].pos= m_Vec3( vert_pos0, z_bottom_top[1] );
	clipped_vertices_[2].pos= m_Vec3( vert_pos1, z_bottom_top[1] );
	clipped_vertices_[3].pos= m_Vec3( vert_pos1, z_bottom_top[0] );
	clipped_vertices_[0].tc= m_Vec2( tc_1 * tc_scale, tc_top );
	clipped_vertices_[1].tc= m_Vec2( tc_1 * tc_scale, float( texture.full_alpha_row[1] << 16u ) );
	clipped_vertices_[2].tc= m_Vec2( tc_0 * tc_scale, float( texture.full_alpha_row[1] << 16u ) );
	clipped_vertices_[3].tc= m_Vec2( tc_0 * tc_scale, tc_top );
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
		return;

	float min_world_z= Constants::max_float, max_world_z= Constants::min_float;
	unsigned int min_worlz_z_vertex= 0u, max_world_z_vertex= 0u;

	RasterizerVertex verties_projected[ c_max_clip_vertices_ ];
	ClippedVertex* v= fisrt_clipped_vertex_;
	for( unsigned int i= 0u; i < polygon_vertex_count; i++, v= v->next )
	{
		if( v->pos.z < min_world_z )
		{
			min_world_z= v->pos.z;
			min_worlz_z_vertex= i;
		}
		if( v->pos.z > max_world_z )
		{
			max_world_z= v->pos.z;
			max_world_z_vertex= i;
		}

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

	if( !is_dynamic_wall && rasterizer_.IsOccluded( verties_projected, polygon_vertex_count ) )
		return;

	int mip= 0;
	const SurfacesCache::Surface* surface;
	if( min_worlz_z_vertex != max_world_z_vertex )
	{
		// Calculate only vertical texture scale.
		// TODO - maybe calculate also horizontal texture scale?
		const fixed16_t dv= verties_projected[ max_world_z_vertex ].v - verties_projected[ min_worlz_z_vertex ].v;
		const fixed16_t dx= verties_projected[ max_world_z_vertex ].x - verties_projected[ min_worlz_z_vertex ].x;
		const fixed16_t dy= verties_projected[ max_world_z_vertex ].y - verties_projected[ min_worlz_z_vertex ].y;
		const fixed8_t d_len_square= FixedMul<16+8>( dx, dx ) + FixedMul<16+8>( dy, dy );
		const int d_tc_d_len_square= FixedMul<16+8>( dv, dv ) / std::max( d_len_square, 1 );

		if( d_tc_d_len_square < 2 * 2 )
		{
			mip= 0;
			surface= GetWallSurface<0>( wall );
		}
		else
		{
			if( d_tc_d_len_square < 4 * 4 )
			{
				mip= 1;
				surface= GetWallSurface<1>( wall );
			}
			else if( d_tc_d_len_square < 8 * 8 )
			{
				surface= GetWallSurface<2>( wall );
				mip= 2;
			}
			else
			{
				surface= GetWallSurface<3>( wall );
				mip= 3;
			}

			for( unsigned int i= 0u; i < polygon_vertex_count; i++ )
			{
				verties_projected[i].u >>= mip;
				verties_projected[i].v >>= mip;
			}
		}
	}
	else
		surface= GetWallSurface<3>( wall );

	rasterizer_.SetTexture( surface->size[0], surface->size[1], surface->GetData() );

	if( is_dynamic_wall )
	{
		if( texture.has_alpha )
			rasterizer_.DrawTexturedConvexPolygonSpanCorrected<
				Rasterizer::DepthTest::Yes, Rasterizer::DepthWrite::Yes,
				Rasterizer::AlphaTest::Yes,
				Rasterizer::OcclusionTest::No, Rasterizer::OcclusionWrite::Yes>( verties_projected, polygon_vertex_count, !is_back );
		else
			rasterizer_.DrawTexturedConvexPolygonSpanCorrected<
				Rasterizer::DepthTest::Yes, Rasterizer::DepthWrite::Yes,
				Rasterizer::AlphaTest::No,
				Rasterizer::OcclusionTest::No, Rasterizer::OcclusionWrite::Yes>( verties_projected, polygon_vertex_count, !is_back );
	}
	else
	{
		if( texture.has_alpha )
			rasterizer_.DrawTexturedConvexPolygonSpanCorrected<
				Rasterizer::DepthTest::No, Rasterizer::DepthWrite::Yes,
				Rasterizer::AlphaTest::Yes,
				Rasterizer::OcclusionTest::Yes, Rasterizer::OcclusionWrite::Yes>( verties_projected, polygon_vertex_count, !is_back );
		else
			rasterizer_.DrawTexturedConvexPolygonSpanCorrected<
				Rasterizer::DepthTest::No, Rasterizer::DepthWrite::Yes,
				Rasterizer::AlphaTest::No,
				Rasterizer::OcclusionTest::Yes, Rasterizer::OcclusionWrite::Yes>( verties_projected, polygon_vertex_count, !is_back );
	}

	rasterizer_.UpdateOcclusionHierarchy( verties_projected, polygon_vertex_count, texture.has_alpha );
}

void MapDrawerSoft::DrawWalls(
	const MapState& map_state,
	const m_Mat4& matrix,
	const m_Vec2& camera_position_xy,
	const ViewClipPlanes& view_clip_planes )
{
	// Draw static walls fron to back, using bsp tree.
	map_bsp_tree_->EnumerateSegmentsFrontToBack(
		camera_position_xy,
		[&]( const MapBSPTree::WallSegment& segment )
		{
			DrawWallSegment<false>(
				static_walls_[ segment.wall_index ],
				segment.vert_pos[0], segment.vert_pos[1], 0.0f,
				segment.start, segment.end,
				matrix, camera_position_xy, view_clip_planes );
		} );


	// TODO - maybe, we can dynamically add dynamic walls to BSP-tree?
	const MapState::DynamicWalls& dynamic_walls= map_state.GetDynamicWalls();
	for( unsigned int w= 0u; w < dynamic_walls_.size(); w++ )
	{
		const MapState::DynamicWall& wall= dynamic_walls[w];
		DrawWall& draw_wall= dynamic_walls_[w];
		if( draw_wall.texture_id != wall.texture_id )
		{
			draw_wall.texture_id= wall.texture_id;

			// Reset surfaces cache for this wall, if texture changed.
			for( SurfacesCache::Surface*& surf_ptr : draw_wall.mips_surfaces )
			{
				if( surf_ptr != nullptr )
				{
					surf_ptr->owner= nullptr;
					surf_ptr= nullptr;
				}
			}
		}

		DrawWallSegment<true>(
			draw_wall,
			wall.vert_pos[0], wall.vert_pos[1], wall.z,
			0.0f, 1.0f,
			matrix, camera_position_xy, view_clip_planes );
	}
}

void MapDrawerSoft::DrawFloorsAndCeilings( const m_Mat4& matrix, const ViewClipPlanes& view_clip_planes  )
{
	for( unsigned int i= 0u; i < map_floors_and_ceilings_.size(); i++ )
	{
		FloorCeilingCell& cell= map_floors_and_ceilings_[i];
		const bool is_ceiling= i >= first_ceiling_;
		const float z= is_ceiling ? GameConstants::walls_height : 0.0f;

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

		if( rasterizer_.IsOccluded( verties_projected, polygon_vertex_count ) )
			continue;

		// Search longest edge for mip calculation.
		unsigned int longest_edge_index= 0u;
		fixed8_t longest_edge_squre_length= 1; // fixed8_t range should be enought for vector ( 2048, 2048 ) square length.
		for( unsigned int i= 0u; i < polygon_vertex_count; i++ )
		{
			unsigned int prev_i= i == 0u ? (polygon_vertex_count - 1u) : (i - 1u);
			const fixed16_t dx= verties_projected[i].x - verties_projected[prev_i].x;
			const fixed16_t dy= verties_projected[i].y - verties_projected[prev_i].y;
			const fixed8_t square_length= FixedMul<16+8>( dx, dx ) + FixedMul<16+8>( dy, dy );
			if( square_length > longest_edge_squre_length )
			{
				longest_edge_squre_length= square_length;
				longest_edge_index= i;
			}
		}
		int mip= 0;
		const SurfacesCache::Surface* surface;
		// Calculate d_tc / d_length for longest edge, select mip.
		unsigned int prev_v= longest_edge_index == 0u ? (polygon_vertex_count - 1u) : (longest_edge_index - 1u);
		const fixed16_t du= verties_projected[longest_edge_index].u - verties_projected[prev_v].u;
		const fixed16_t dv= verties_projected[longest_edge_index].v - verties_projected[prev_v].v;
		const fixed8_t square_tc_delta= FixedMul<16+8>( du, du ) + FixedMul<16+8>( dv, dv );
		const int d_tc_d_len_square = square_tc_delta / longest_edge_squre_length;

		if( d_tc_d_len_square < 1 * 1 )
		{
			mip= 0;
			surface= GetFloorCeilingSurface<0>( cell );
		}
		else
		{
			if( d_tc_d_len_square < 2 * 2 )
			{
				mip= 1;
				surface= GetFloorCeilingSurface<1>( cell );
			}
			else if( d_tc_d_len_square < 4 * 4 )
			{
				mip= 2;
				surface= GetFloorCeilingSurface<2>( cell );
			}
			else
			{
				mip= 3;
				surface= GetFloorCeilingSurface<3>( cell );
			}

			for( unsigned int i= 0u; i < polygon_vertex_count; i++ )
			{
				verties_projected[i].u >>= mip;
				verties_projected[i].v >>= mip;
			}
		}

		rasterizer_.SetTexture(
			surface->size[0], surface->size[1],
			surface->GetData() );

		rasterizer_.DrawTexturedConvexPolygonPerLineCorrected<
			Rasterizer::DepthTest::No, Rasterizer::DepthWrite::Yes,
			Rasterizer::AlphaTest::No,
			Rasterizer::OcclusionTest::Yes, Rasterizer::OcclusionWrite::Yes>( verties_projected, polygon_vertex_count, is_ceiling );

		// TODO - does this needs?
		// Maybe update whole screen hierarchy after floors and ceilings?
		rasterizer_.UpdateOcclusionHierarchy( verties_projected, polygon_vertex_count, false );
	}
}

void MapDrawerSoft::DrawModel(
	const ModelsGroup& models_group,
	const std::vector<Model>& model_group_models,
	const unsigned int model_id,
	const unsigned int animation_frame,
	const ViewClipPlanes& view_clip_planes,
	const m_Vec3& position,
	const m_Mat4& rotation_matrix,
	const m_Mat4& view_matrix,
	const m_Vec3& camera_position,
	const unsigned char visible_groups_mask,
	const bool transparent,
	const bool fullbright,
	const unsigned int submodel_id,
	const unsigned char color )
{
	const Model& base_model= model_group_models[ model_id ];
	const Submodel& model= (submodel_id == ~0u) ? base_model : base_model.submodels[ submodel_id ];
	const std::vector<unsigned short>& indeces= transparent ? model.transparent_triangles_indeces : model.regular_triangles_indeces;
	if( indeces.size() == 0u )
		return;

	unsigned int active_clip_planes_mask= 0u;

	PC_ASSERT( animation_frame < model.frame_count );
	const m_BBox3& bbox= model.animations_bboxes[ animation_frame ];

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

	// Calculate final matrix.
	const m_Mat4 to_world_mat= rotation_matrix * translate_mat;
	const m_Mat4 final_mat= to_world_mat * view_matrix;

	// Select triangle rasterization func.
	// Calculate bounding box w_min/w_max.
	// TODO - maybe use clipped bounding box? Maybe use maximum polygon size of model for w_ratio calculation?

	float x_min= Constants::max_float, x_max= Constants::min_float;
	float y_min= Constants::max_float, y_max= Constants::min_float;
	float w_min= Constants::max_float, w_max= Constants::min_float;
	for( unsigned int z= 0u; z < 2u; z++ )
	for( unsigned int y= 0u; y < 2u; y++ )
	for( unsigned int x= 0u; x < 2u; x++ )
	{
		const m_Vec3 point(
			x == 0 ? bbox.min.x : bbox.max.x,
			y == 0 ? bbox.min.y : bbox.max.y,
			z == 0 ? bbox.min.z : bbox.max.z );

		const float w= point.x * final_mat.value[3] + point.y * final_mat.value[7] + point.z * final_mat.value[11] + final_mat.value[15];
		if( w < w_min ) w_min= w;
		if( w > w_max ) w_max= w;

		if( w > 0.0f )
		{
			m_Vec2 vertex_projected= ( point * final_mat ).xy();
			vertex_projected/= w;
			const float screen_x= ( vertex_projected.x + 1.0f ) * screen_transform_x_;
			const float screen_y= ( vertex_projected.y + 1.0f ) * screen_transform_y_;
			if( screen_x < x_min ) x_min= screen_x;
			if( screen_x > x_max ) x_max= screen_x;
			if( screen_y < y_min ) y_min= screen_y;
			if( screen_y > y_max ) y_max= screen_y;
		}
	}

	// Try to reject model, using hierarchical depth-test.
	// Model must be not so near for thist test - farther, then z_near.
	if( w_min > 1.1f / float( 1u << Rasterizer::c_max_inv_z_min_log2 ) )
	{
		PC_ASSERT( w_max >= w_min );

		x_min= std::min( std::max( x_min, 0.0f ), screen_transform_x_ * 2.0f );
		y_min= std::min( std::max( y_min, 0.0f ), screen_transform_y_ * 2.0f );
		x_max= std::min( std::max( x_max, 0.0f ), screen_transform_x_ * 2.0f );
		y_max= std::min( std::max( y_max, 0.0f ), screen_transform_y_ * 2.0f );
		if( rasterizer_.IsDepthOccluded(
			fixed16_t(x_min * 65536.0f), fixed16_t(y_min * 65536.0f),
			fixed16_t(x_max * 65536.0f), fixed16_t(y_max * 65536.0f),
			fixed16_t(w_min * 65536.0f), fixed16_t(w_max * 65536.0f) ) )
			return;
	}

	Rasterizer::TriangleDrawFunc draw_func, alpha_draw_func;

	if( transparent )
	{
		draw_func=
			&Rasterizer::DrawTexturedTriangleSpanCorrected<
				Rasterizer::DepthTest::Yes, Rasterizer::DepthWrite::Yes,
				Rasterizer::AlphaTest::No,
				Rasterizer::OcclusionTest::No, Rasterizer::OcclusionWrite::No,
				Rasterizer::Lighting::Yes, Rasterizer::Blending::Yes>;
		alpha_draw_func=
			&Rasterizer::DrawTexturedTriangleSpanCorrected<
				Rasterizer::DepthTest::Yes, Rasterizer::DepthWrite::Yes,
				Rasterizer::AlphaTest::Yes,
				Rasterizer::OcclusionTest::No, Rasterizer::OcclusionWrite::No,
				Rasterizer::Lighting::Yes, Rasterizer::Blending::Yes>;
	}
	else
	{
		draw_func=
			&Rasterizer::DrawTexturedTriangleSpanCorrected<
				Rasterizer::DepthTest::Yes, Rasterizer::DepthWrite::Yes,
				Rasterizer::AlphaTest::No,
				Rasterizer::OcclusionTest::No, Rasterizer::OcclusionWrite::No,
				Rasterizer::Lighting::Yes, Rasterizer::Blending::No>;
		alpha_draw_func=
			&Rasterizer::DrawTexturedTriangleSpanCorrected<
				Rasterizer::DepthTest::Yes, Rasterizer::DepthWrite::Yes,
				Rasterizer::AlphaTest::Yes,
				Rasterizer::OcclusionTest::No, Rasterizer::OcclusionWrite::No,
				Rasterizer::Lighting::Yes, Rasterizer::Blending::No>;
	}

	if( w_min > 0.0f /* && w_max > 0.0f */ )
	{
		// If 'w' variation is small - draw model triangles with affine texturing, else - use perspective correction.
		const float c_ratio_threshold= 1.2f; // 20 %
		const float ratio= w_max / w_min;
		if( ratio < c_ratio_threshold )
		{
			if( transparent )
			{
				draw_func= &Rasterizer::DrawAffineTexturedTriangle<
					Rasterizer::DepthTest::Yes, Rasterizer::DepthWrite::Yes,
					Rasterizer::AlphaTest::No,
					Rasterizer::OcclusionTest::No, Rasterizer::OcclusionWrite::No,
					Rasterizer::Lighting::Yes, Rasterizer::Blending::Yes>;
				alpha_draw_func= &Rasterizer::DrawAffineTexturedTriangle<
					Rasterizer::DepthTest::Yes, Rasterizer::DepthWrite::Yes,
					Rasterizer::AlphaTest::Yes,
					Rasterizer::OcclusionTest::No, Rasterizer::OcclusionWrite::No,
					Rasterizer::Lighting::Yes, Rasterizer::Blending::Yes>;
			}
			else
			{
				draw_func= &Rasterizer::DrawAffineTexturedTriangle<
					Rasterizer::DepthTest::Yes, Rasterizer::DepthWrite::Yes,
					Rasterizer::AlphaTest::No,
					Rasterizer::OcclusionTest::No, Rasterizer::OcclusionWrite::No,
					Rasterizer::Lighting::Yes, Rasterizer::Blending::No>;
				alpha_draw_func= &Rasterizer::DrawAffineTexturedTriangle<
					Rasterizer::DepthTest::Yes, Rasterizer::DepthWrite::Yes,
					Rasterizer::AlphaTest::Yes,
					Rasterizer::OcclusionTest::No, Rasterizer::OcclusionWrite::No,
					Rasterizer::Lighting::Yes, Rasterizer::Blending::No>;
			}
		}
	}

	const m_Vec3 cam_pos_model_space= ( camera_position - position ) * inv_rotation_mat;

	const unsigned int first_animation_vertex= model.animations_vertices.size() / model.frame_count * animation_frame;

	if( &models_group == &monsters_models_ && model_id == 0u )
	{
		// Detect player - set colored texture.
		const TextureView texture_view= GetPlayerTexture( color );
		rasterizer_.SetTexture( texture_view.size[0], texture_view.size[1], texture_view.data );
	}
	else
	{
		const ModelsGroup::ModelEntry& model_entry= models_group.models[ model_id ];
		rasterizer_.SetTexture(
			model_entry.texture_size[0], model_entry.texture_size[1],
			models_group.textures_data.data() + model_entry.texture_data_offset );
	}

	// TODO - make other branch, if clip_planes_transformed_count == 0
	// Transform animation vertices, then rasterize trianglez directly, without clipping.

	// TODO - use original QUADS from .3o/.car models.

	for( unsigned int t= 0u; t < indeces.size(); t+= 3u )
	{
		const Model::Vertex& first_vertex= model.vertices[ indeces[t] ];

		if( ( first_vertex.groups_mask & visible_groups_mask ) == 0u )
			continue;

		m_Vec3 triangle_center( 0.0f, 0.0f, 0.0f );
		for( unsigned int tv= 0u; tv < 3u; tv++ )
		{
			const Model::Vertex& vertex= model.vertices[ indeces[t + tv] ];
			const Model::AnimationVertex& animation_vertex= model.animations_vertices[ first_animation_vertex + vertex.vertex_id ];

			clipped_vertices_[tv].pos= m_Vec3( float(animation_vertex.pos[0]), float(animation_vertex.pos[1]), float(animation_vertex.pos[2]) ) / 2048.0f;
			clipped_vertices_[tv].tc.x= vertex.tex_coord[0] * float(base_model.texture_size[0]) * 65536.0f;
			clipped_vertices_[tv].tc.y= vertex.tex_coord[1] * float(base_model.texture_size[1]) * 65536.0f;

			triangle_center+= clipped_vertices_[tv].pos;
		}
		{ // Try reject back faces
			const m_Vec3 v0= clipped_vertices_[1].pos - clipped_vertices_[0].pos;
			const m_Vec3 v1= clipped_vertices_[2].pos - clipped_vertices_[0].pos;
			const m_Vec3 vec_to_cam= cam_pos_model_space - clipped_vertices_[0].pos;
			if( mVec3Cross( v0, v1 ) * vec_to_cam < 0.0f )
				continue;
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

		fixed16_t light= g_fixed16_one;
		if( !fullbright )
		{
			triangle_center*= 1.0f / 3.0f;
			const m_Vec2 triangle_center_world_space= ( triangle_center * to_world_mat ).xy();
			const unsigned int lightmap_x= static_cast<unsigned int>( triangle_center_world_space.x * float(MapData::c_lightmap_scale) );
			const unsigned int lightmap_y= static_cast<unsigned int>( triangle_center_world_space.y * float(MapData::c_lightmap_scale) );

			if( lightmap_x < MapData::c_lightmap_size && lightmap_y < MapData::c_lightmap_size )
				light= ScaleLightmapLight( current_map_data_->lightmap[ lightmap_x + lightmap_y * MapData::c_lightmap_size ] );
		}
		rasterizer_.SetLight( light );

		const bool triangle_needs_alpha_test= first_vertex.alpha_test_mask != 0u;
		const Rasterizer::TriangleDrawFunc triangle_func= triangle_needs_alpha_test ? alpha_draw_func : draw_func;

		RasterizerVertex traingle_vertices[3];
		traingle_vertices[0]= verties_projected[0];
		for( unsigned int i= 0u; i < polygon_vertex_count - 2u; i++ )
		{
			traingle_vertices[1]= verties_projected[ i + 1u ];
			traingle_vertices[2]= verties_projected[ i + 2u ];
			(rasterizer_.*triangle_func)( traingle_vertices );
		}
	} // for model triangles
}

void MapDrawerSoft::DrawModelShadow(
	const Model& base_model,
	const unsigned int animation_frame,
	const ViewClipPlanes& view_clip_planes,
	const m_Vec3& position,
	const m_Mat4& rotation_matrix,
	const m_Mat4& view_matrix,
	const m_Vec3& camera_position,
	const m_Vec3& light_pos,
	const unsigned char visible_groups_mask,
	const unsigned int submodel_id )
{
	const float c_shadow_z_offset= 0.02f;

	const Submodel& model= (submodel_id == ~0u) ? base_model : base_model.submodels[ submodel_id ];
	const std::vector<unsigned short>& indeces=  model.regular_triangles_indeces;
	if( indeces.size() == 0u )
		return;

	m_Mat4 inv_rotation_mat= rotation_matrix;
	inv_rotation_mat.Transpose(); // For rotation matrix transpose is euqivalent for inverse.

	const m_Vec3 light_pos_model_space= ( light_pos - position ) * inv_rotation_mat;

	PC_ASSERT( animation_frame < model.frame_count );
	const m_BBox3& bbox= model.animations_bboxes[ animation_frame ];

	const auto project_model_vertex=
	[&]( const m_Vec3& v ) -> m_Vec3
	{
		const m_Vec3 vec_from_light= v - light_pos_model_space;
		return m_Vec3( light_pos_model_space.xy() + vec_from_light.xy() / vec_from_light.z * (-light_pos_model_space.z), c_shadow_z_offset );
	};

	// Project bouning box to 2d bounding box.
	m_BBox2 bbox_projected;
	bbox_projected.min= bbox_projected.max= project_model_vertex( bbox.min ).xy();
	for( unsigned int z= 0u; z < 2u; z++ )
	for( unsigned int y= 0u; y < 2u; y++ )
	for( unsigned int x= 0u; x < 2u; x++ )
	{
		const m_Vec3 point(
			x == 0 ? bbox.min.x : bbox.max.x,
			y == 0 ? bbox.min.y : bbox.max.y,
			z == 0 ? bbox.min.z : bbox.max.z );

		bbox_projected+= project_model_vertex( point ).xy();
	}

	unsigned int active_clip_planes_mask= 0u;

	m_Mat4 translate_mat, bbox_mat;
	translate_mat.Translate( position );
	bbox_mat= rotation_matrix * translate_mat;

	// Clip-planes bounding box test
	for( const m_Plane3& clip_plane : view_clip_planes )
	{
		unsigned int vertices_inside= 0u;
		for( unsigned int y= 0u; y < 2u; y++ )
		for( unsigned int x= 0u; x < 2u; x++ )
		{
			const m_Vec3 point(
				x == 0 ? bbox_projected.min.x : bbox_projected.max.x,
				y == 0 ? bbox_projected.min.y : bbox_projected.max.y,
				c_shadow_z_offset );

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

	// Calculate final matrix.
	const m_Mat4 to_world_mat= rotation_matrix * translate_mat;
	const m_Mat4 final_mat= to_world_mat * view_matrix;

	// Calculate screen-space bounding box.
	float x_min= Constants::max_float, x_max= Constants::min_float;
	float y_min= Constants::max_float, y_max= Constants::min_float;
	float w_min= Constants::max_float, w_max= Constants::min_float;
	for( unsigned int y= 0u; y < 2u; y++ )
	for( unsigned int x= 0u; x < 2u; x++ )
	{
		const m_Vec3 point(
			x == 0 ? bbox_projected.min.x : bbox_projected.max.x,
			y == 0 ? bbox_projected.min.y : bbox_projected.max.y,
			c_shadow_z_offset );

		const float w= point.x * final_mat.value[3] + point.y * final_mat.value[7] + point.z * final_mat.value[11] + final_mat.value[15];
		if( w < w_min ) w_min= w;
		if( w > w_max ) w_max= w;

		if( w > 0.0f )
		{
			m_Vec2 vertex_projected= ( point * final_mat ).xy();
			vertex_projected/= w;
			const float screen_x= ( vertex_projected.x + 1.0f ) * screen_transform_x_;
			const float screen_y= ( vertex_projected.y + 1.0f ) * screen_transform_y_;
			if( screen_x < x_min ) x_min= screen_x;
			if( screen_x > x_max ) x_max= screen_x;
			if( screen_y < y_min ) y_min= screen_y;
			if( screen_y > y_max ) y_max= screen_y;
		}
	}

	// Try to reject model, using hierarchical depth-test.
	// Model must be not so near for thist test - farther, then z_near.
	if( w_min > 1.1f / float( 1u << Rasterizer::c_max_inv_z_min_log2 ) )
	{
		PC_ASSERT( w_max >= w_min );

		x_min= std::min( std::max( x_min, 0.0f ), screen_transform_x_ * 2.0f );
		y_min= std::min( std::max( y_min, 0.0f ), screen_transform_y_ * 2.0f );
		x_max= std::min( std::max( x_max, 0.0f ), screen_transform_x_ * 2.0f );
		y_max= std::min( std::max( y_max, 0.0f ), screen_transform_y_ * 2.0f );
		if( rasterizer_.IsDepthOccluded(
			fixed16_t(x_min * 65536.0f), fixed16_t(y_min * 65536.0f),
			fixed16_t(x_max * 65536.0f), fixed16_t(y_max * 65536.0f),
			fixed16_t(w_min * 65536.0f), fixed16_t(w_max * 65536.0f) ) )
			return;
	}

	const m_Vec3 cam_pos_model_space= ( camera_position - position ) * inv_rotation_mat;
	if( cam_pos_model_space.z < 0.0f )
		return; // We can not see shadow from bottom.

	const unsigned int first_animation_vertex= model.animations_vertices.size() / model.frame_count * animation_frame;

	// Draw shadow triangles.
	for( unsigned int t= 0u; t < indeces.size(); t+= 3u )
	{
		const Model::Vertex& first_vertex= model.vertices[ indeces[t] ];

		if( ( first_vertex.groups_mask & visible_groups_mask ) == 0u )
			continue;

		m_Vec3 triangle_center( 0.0f, 0.0f, 0.0f );
		for( unsigned int tv= 0u; tv < 3u; tv++ )
		{
			const Model::Vertex& vertex= model.vertices[ indeces[t + tv] ];
			const Model::AnimationVertex& animation_vertex= model.animations_vertices[ first_animation_vertex + vertex.vertex_id ];
			const m_Vec3 vert_pos= m_Vec3( float(animation_vertex.pos[0]), float(animation_vertex.pos[1]), float(animation_vertex.pos[2]) ) / 2048.0f;

			clipped_vertices_[tv].pos= project_model_vertex( vert_pos );
			clipped_vertices_[tv].tc.x= 0.0f;
			clipped_vertices_[tv].tc.y= 0.0f;

			triangle_center+= clipped_vertices_[tv].pos;
		}
		{ // Try reject back faces
			const m_Vec3 v0= clipped_vertices_[1].pos - clipped_vertices_[0].pos;
			const m_Vec3 v1= clipped_vertices_[2].pos - clipped_vertices_[0].pos;
			const m_Vec3 vec_to_cam= cam_pos_model_space - clipped_vertices_[0].pos;
			if( mVec3Cross( v0, v1 ) * vec_to_cam < 0.0f )
				continue;
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
			out_v.u= 0;
			out_v.v= 0;
			out_v.z= fixed16_t( w * 65536.0f );
		}

		RasterizerVertex traingle_vertices[3];
		traingle_vertices[0]= verties_projected[0];
		for( unsigned int i= 0u; i < polygon_vertex_count - 2u; i++ )
		{
			traingle_vertices[1]= verties_projected[ i + 1u ];
			traingle_vertices[2]= verties_projected[ i + 2u ];
			rasterizer_.DrawShadowTriangle( traingle_vertices );
		}
	} // for model triangles
}

void MapDrawerSoft::DrawSky(
	const m_Mat4& matrix,
	const m_Vec3& sky_pos,
	const ViewClipPlanes& view_clip_planes )
{
	PC_ASSERT( sky_texture_.file_name[0] != '\0' );
	PC_ASSERT( sky_texture_.size[0] > 0 && sky_texture_.size[1] > 0 );

	constexpr int c_repeat_x= 5;
	constexpr int c_repeat_y= 3;
	constexpr int c_x_polygons= c_repeat_x * 4;
	constexpr int c_y_polygons= c_repeat_y * 3;
	constexpr int c_y_polygons_start= -1; // TODO - does this need ? maybe, sky abowe horizont should be enough?
	constexpr float c_radius= float(MapData::c_map_size);

	const fixed16_t tex_size_x= fixed16_t( sky_texture_.size[0] << 16u );
	const fixed16_t tex_size_y= fixed16_t( sky_texture_.size[1] << 16u );

	rasterizer_.SetTexture(
		sky_texture_.size[0], sky_texture_.size[1],
		sky_texture_.data.data() );

	// TODO - optimize this
	// 180 quads is too many for sky.
	for( int y= c_y_polygons_start; y < c_y_polygons; y++ )
	for( int x= 0; x < c_x_polygons; x++ )
	{
		const float angle_x= float(x) * ( Constants:: two_pi / float(c_x_polygons) );
		const float angle_y= float(y) * ( Constants::half_pi / float(c_y_polygons) );
		const float next_angle_x= float(x+1) * ( Constants:: two_pi / float(c_x_polygons) );
		const float next_angle_y= float(y+1) * ( Constants::half_pi / float(c_y_polygons) );

		clipped_vertices_[0].pos= m_Vec3( std::cos(     angle_x) * std::cos(     angle_y), std::sin(     angle_x) * std::cos(     angle_y), std::sin(     angle_y) ) * c_radius + sky_pos;
		clipped_vertices_[1].pos= m_Vec3( std::cos(next_angle_x) * std::cos(     angle_y), std::sin(next_angle_x) * std::cos(     angle_y), std::sin(     angle_y) ) * c_radius + sky_pos;
		clipped_vertices_[2].pos= m_Vec3( std::cos(next_angle_x) * std::cos(next_angle_y), std::sin(next_angle_x) * std::cos(next_angle_y), std::sin(next_angle_y) ) * c_radius + sky_pos;
		clipped_vertices_[3].pos= m_Vec3( std::cos(     angle_x) * std::cos(next_angle_y), std::sin(     angle_x) * std::cos(next_angle_y), std::sin(next_angle_y) ) * c_radius + sky_pos;

		int tc_start[2], tc_end[2];
		tc_start[0]= ( x * tex_size_x * c_repeat_x / c_x_polygons ) % tex_size_x;
		if( tc_start[0] < 0 ) tc_start[0]+= tex_size_x;
		tc_start[1]= ( ( c_y_polygons - y - 1 ) * tex_size_y * c_repeat_y / c_y_polygons ) % tex_size_y;
		if( tc_start[1] < 0 ) tc_start[1]+= tex_size_y;
		tc_end[0]= tc_start[0] + tex_size_x * c_repeat_x / c_x_polygons;
		tc_end[1]= tc_start[1] + tex_size_y * c_repeat_y / c_y_polygons;

		clipped_vertices_[0].tc= m_Vec2( float(tc_start[0]), float(tc_end  [1]) );
		clipped_vertices_[1].tc= m_Vec2( float(tc_end  [0]), float(tc_end  [1]) );
		clipped_vertices_[2].tc= m_Vec2( float(tc_end  [0]), float(tc_start[1]) );
		clipped_vertices_[3].tc= m_Vec2( float(tc_start[0]), float(tc_start[1]) );

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

		if( rasterizer_.IsOccluded( verties_projected, polygon_vertex_count ) )
			continue;

		rasterizer_.DrawTexturedConvexPolygonSpanCorrected<
			Rasterizer::DepthTest::No, Rasterizer::DepthWrite::No,
			Rasterizer::AlphaTest::No,
			Rasterizer::OcclusionTest::Yes, Rasterizer::OcclusionWrite::No>( verties_projected, polygon_vertex_count, true );
	}
}

void MapDrawerSoft::DrawEffectsSprites(
	const MapState& map_state,
	const m_Mat4& view_matrix,
	const m_Vec3& camera_position,
	const ViewClipPlanes& view_clip_planes )
{
	SortEffectsSprites( map_state.GetSpriteEffects(), camera_position, sorted_sprites_ );

	for( const MapState::SpriteEffect* const sprite_ptr : sorted_sprites_ )
	{
		const MapState::SpriteEffect& sprite= *sprite_ptr;

		const GameResources::SpriteEffectDescription& sprite_description= game_resources_->sprites_effects_description[ sprite.effect_id ];
		const SpriteTexture& sprite_texture= sprite_effects_textures_[ sprite.effect_id ];

		// TODO - optimize this. Use less matrix multiplications.
		// TODO - maybe add hierarchical depth-test ?

		const m_Vec3 vec_to_sprite= sprite.pos - camera_position;
		float sprite_angles[2];
		VecToAngles( vec_to_sprite, sprite_angles );

		m_Mat4 rotate_z, rotate_x, shift_mat, sprite_mat;
		rotate_z.RotateZ( sprite_angles[0] - Constants::half_pi );
		rotate_x.RotateX( sprite_angles[1] );
		shift_mat.Translate( sprite.pos );
		sprite_mat= rotate_x * rotate_z * shift_mat;

		const float additional_scale= ( sprite_description.half_size ? 0.5f : 1.0f ) / 128.0f;
		const float size_x= additional_scale * float(sprite_texture.size[0]);
		const float size_z= additional_scale * float(sprite_texture.size[1]) ;

		clipped_vertices_[0].pos= m_Vec3( -size_x, 0.0f, -size_z ) * sprite_mat;
		clipped_vertices_[1].pos= m_Vec3( +size_x, 0.0f, -size_z ) * sprite_mat;
		clipped_vertices_[2].pos= m_Vec3( +size_x, 0.0f, +size_z ) * sprite_mat;
		clipped_vertices_[3].pos= m_Vec3( -size_x, 0.0f, +size_z ) * sprite_mat;
		clipped_vertices_[0].tc= m_Vec2( 0.0f, 0.0f );
		clipped_vertices_[1].tc= m_Vec2( float(sprite_texture.size[0] << 16), 0.0f );
		clipped_vertices_[2].tc= m_Vec2( float(sprite_texture.size[0] << 16), float(sprite_texture.size[1] << 16) );
		clipped_vertices_[3].tc= m_Vec2( 0.0f, float(sprite_texture.size[1] << 16) );
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
			m_Vec3 vertex_projected= v->pos * view_matrix;
			const float w= v->pos.x * view_matrix.value[3] + v->pos.y * view_matrix.value[7] + v->pos.z * view_matrix.value[11] + view_matrix.value[15];

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

		const unsigned int frame= static_cast<unsigned int>( sprite.frame ) % sprite_texture.size[2];
		rasterizer_.SetTexture(
			sprite_texture.size[0], sprite_texture.size[1],
			sprite_texture.data.data() + sprite_texture.size[0] * sprite_texture.size[1] * frame );

		Rasterizer::ConvexPolygonDrawFunc draw_func;

		if( !sprite_description.light_on )
		{
			fixed16_t light= g_fixed16_one;
			const unsigned int lightmap_x= static_cast<unsigned int>( sprite.pos.x * float(MapData::c_lightmap_scale) );
			const unsigned int lightmap_y= static_cast<unsigned int>( sprite.pos.y * float(MapData::c_lightmap_scale) );
			if( lightmap_x < MapData::c_lightmap_size && lightmap_y < MapData::c_lightmap_size )
				light= ScaleLightmapLight( current_map_data_->lightmap[ lightmap_x + lightmap_y * MapData::c_lightmap_size ] );
			rasterizer_.SetLight( light );

			draw_func=
				&Rasterizer::DrawTexturedConvexPolygonSpanCorrected<
					Rasterizer::DepthTest::Yes, Rasterizer::DepthWrite::Yes,
					Rasterizer::AlphaTest::Yes,
					Rasterizer::OcclusionTest::No, Rasterizer::OcclusionWrite::No,
					Rasterizer::Lighting::Yes, Rasterizer::Blending::Yes>;
		}
		else
			draw_func=
				&Rasterizer::DrawTexturedConvexPolygonSpanCorrected<
					Rasterizer::DepthTest::Yes, Rasterizer::DepthWrite::Yes,
					Rasterizer::AlphaTest::Yes,
					Rasterizer::OcclusionTest::No, Rasterizer::OcclusionWrite::No,
					Rasterizer::Lighting::No, Rasterizer::Blending::Yes>;

		(rasterizer_.*draw_func)( verties_projected, polygon_vertex_count, false );
	}
}

void MapDrawerSoft::DrawBMPObjectsSprites(
	const MapState& map_state,
	const m_Mat4& view_matrix,
	const m_Vec3& camera_position,
	const ViewClipPlanes& view_clip_planes )
{
	const float sprites_frame= map_state.GetSpritesFrame();

	// TODO - maybe add hierarchical depth test?
	for( const MapState::StaticModel& model : map_state.GetStaticModels() )
	{
		if( model.model_id >= current_map_data_->models_description.size() )
			continue;

		const MapData::ModelDescription& model_description= current_map_data_->models_description[ model.model_id ];
		const int bmp_obj_id= model_description.bobj - 1u;
		if( bmp_obj_id < 0 || bmp_obj_id > static_cast<int>( game_resources_->bmp_objects_description.size() ) )
			continue;

		const GameResources::BMPObjectDescription& bmp_description= game_resources_->bmp_objects_description[ bmp_obj_id ];
		const ObjSprite& sprite_picture= game_resources_->bmp_objects_sprites[ bmp_obj_id ];
		const SpriteTexture& sprite_texture= bmp_objects_sprites_[ bmp_obj_id ];

		const float additional_scale= ( bmp_description.half_size ? 0.5f : 1.0f ) / 128.0f;
		const m_Vec3 scale_vec(
			float(sprite_texture.size[0]) * additional_scale,
			1.0f,
			float(sprite_texture.size[1]) * additional_scale );

		m_Vec3 pos= model.pos;
		pos.z+= float( model_description.bmpz ) / 64.0f + scale_vec.z;

		const m_Vec3 vec_to_sprite= pos - camera_position;
		float sprite_angles[2];
		VecToAngles( vec_to_sprite, sprite_angles );

		m_Mat4 rotate_z, shift_mat, sprite_mat;
		rotate_z.RotateZ( sprite_angles[0] - Constants::half_pi );
		shift_mat.Translate( pos );
		sprite_mat= rotate_z * shift_mat;

		clipped_vertices_[0].pos= m_Vec3( -scale_vec.x, 0.0f, -scale_vec.z ) * sprite_mat;
		clipped_vertices_[1].pos= m_Vec3( +scale_vec.x, 0.0f, -scale_vec.z ) * sprite_mat;
		clipped_vertices_[2].pos= m_Vec3( +scale_vec.x, 0.0f, +scale_vec.z ) * sprite_mat;
		clipped_vertices_[3].pos= m_Vec3( -scale_vec.x, 0.0f, +scale_vec.z ) * sprite_mat;
		clipped_vertices_[0].tc= m_Vec2( 0.0f, 0.0f );
		clipped_vertices_[1].tc= m_Vec2( float(sprite_texture.size[0] << 16), 0.0f );
		clipped_vertices_[2].tc= m_Vec2( float(sprite_texture.size[0] << 16), float(sprite_texture.size[1] << 16) );
		clipped_vertices_[3].tc= m_Vec2( 0.0f, float(sprite_texture.size[1] << 16) );
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
			m_Vec3 vertex_projected= v->pos * view_matrix;
			const float w= v->pos.x * view_matrix.value[3] + v->pos.y * view_matrix.value[7] + v->pos.z * view_matrix.value[11] + view_matrix.value[15];

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

		const unsigned int phase= GetModelBMPSpritePhase( model );
		const unsigned int frame= static_cast<unsigned int>( sprites_frame + phase ) % sprite_picture.frame_count;

		rasterizer_.SetTexture(
			sprite_texture.size[0], sprite_texture.size[1],
			sprite_texture.data.data() + sprite_texture.size[0] * sprite_texture.size[1] * frame );

		rasterizer_.DrawTexturedConvexPolygonSpanCorrected<
			Rasterizer::DepthTest::Yes, Rasterizer::DepthWrite::Yes,
			Rasterizer::AlphaTest::Yes,
			Rasterizer::OcclusionTest::No, Rasterizer::OcclusionWrite::No,
			Rasterizer::Lighting::No, Rasterizer::Blending::Yes>
				( verties_projected, polygon_vertex_count, false );
	}
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

template<unsigned int mip>
const SurfacesCache::Surface* MapDrawerSoft::GetWallSurface( DrawWall& wall )
{
	PC_ASSERT( mip < 4u );
	PC_ASSERT( wall.texture_id < MapData::c_max_walls_textures );

	if( wall.mips_surfaces[mip] != nullptr )
		return wall.mips_surfaces[mip];

	const WallTexture& texture= wall_textures_[wall.texture_id];

	// Do not generate cache pixels for alpha-texels.
	// TODO - maybe cut surface below full_alpha_row[0] too?
	const unsigned int y_start= texture.full_alpha_row[0] >> mip;
	const unsigned int y_end= ( texture.full_alpha_row[1] + ( (1u << mip) - 1u ) ) >> mip;

	const unsigned int surface_height= y_end;
	const unsigned int surface_width = wall.surface_width >> mip;
	const unsigned int lightmap_x_shift= ( wall.surface_width == 128u ? 4u : 3u ) - mip;

	surfaces_cache_.AllocateSurface( surface_width, surface_height, &wall.mips_surfaces[mip] );
	SurfacesCache::Surface* const surface= wall.mips_surfaces[mip];
	uint32_t* const out_data= surface->GetData();

	const uint32_t* in_data;
	if( mip == 0u )
		in_data= texture.mip0;
	if( mip == 1u )
		in_data= texture.mips[0];
	if( mip == 2u )
		in_data= texture.mips[1];
	if( mip == 3u )
		in_data= texture.mips[2];

	const unsigned int texture_width= texture.size[0] >> mip;
	const unsigned int texture_x_wrap_mask= texture_width - 1u;

	fixed16_t lightmap_scaled[8];
	for( unsigned int i= 0u; i < 8u; i++ )
		lightmap_scaled[i]= ScaleLightmapLight( wall.lightmap[i] );

	// TODO - check twice lightmap fetching.
	for( unsigned int y= y_start; y < y_end; y++ )
	for( unsigned int x= 0u; x < surface_width ; x++ )
	{
		const uint32_t texel= in_data[ ( x & texture_x_wrap_mask ) + y * texture_width ];
		const fixed16_t light= lightmap_scaled[ (x >> lightmap_x_shift) ];
		unsigned char components[4];
		for( unsigned int i= 0u; i < 3u; i++ )
		{
			const unsigned int c= reinterpret_cast<const unsigned char*>(&texel)[i] * static_cast<unsigned int>(light) >> 16u;
			components[i]= std::min( c, 255u );
		}

		components[3]= reinterpret_cast<const unsigned char*>(&texel)[3];

		std::memcpy( &out_data[ x + y * surface_width ], components, sizeof(uint32_t) );
	}

	return surface;
}

template<unsigned int mip>
const SurfacesCache::Surface* MapDrawerSoft::GetFloorCeilingSurface( FloorCeilingCell& cell )
{
	PC_ASSERT( mip < 4u );

	if( cell.mips_surfaces[mip] != nullptr )
		return cell.mips_surfaces[mip];

	PC_ASSERT( cell.xy[0] < MapData::c_map_size );
	PC_ASSERT( cell.xy[1] < MapData::c_map_size );
	PC_ASSERT( cell.texture_id < MapData::c_floors_textures_count );

	const unsigned int texture_size= MapData::c_floor_texture_size >> mip;
	const unsigned int monolighted_block_size= ( MapData::c_floor_texture_size / MapData::c_lightmap_scale ) >> mip;

	surfaces_cache_.AllocateSurface( texture_size, texture_size, &cell.mips_surfaces[mip] );
	SurfacesCache::Surface* const surface= cell.mips_surfaces[mip];
	uint32_t* const out_data= surface->GetData();

	const uint32_t* in_data;
	if( mip == 0u )
		in_data= floor_textures_[cell.texture_id].data;
	if( mip == 1u )
		in_data= floor_textures_[cell.texture_id].mip1;
	if( mip == 2u )
		in_data= floor_textures_[cell.texture_id].mip2;
	if( mip == 3u )
		in_data= floor_textures_[cell.texture_id].mip3;

	for( unsigned int lightmap_cell_y= 0u; lightmap_cell_y < MapData::c_lightmap_scale; lightmap_cell_y++ )
	for( unsigned int lightmap_cell_x= 0u; lightmap_cell_x < MapData::c_lightmap_scale; lightmap_cell_x++ )
	{
		const unsigned int lightmap_global_x= lightmap_cell_x + MapData::c_lightmap_scale * cell.xy[0];
		const unsigned int lightmap_global_y= lightmap_cell_y + MapData::c_lightmap_scale * cell.xy[1];

		// TODO - Maybe scale light?
		const unsigned char lightmap_value= current_map_data_->lightmap[ lightmap_global_x + lightmap_global_y * MapData::c_lightmap_size ];
		const fixed16_t light= ScaleLightmapLight( lightmap_value );

		for( unsigned int texel_y= 0u; texel_y < monolighted_block_size; texel_y++ )
		for( unsigned int texel_x= 0u; texel_x < monolighted_block_size; texel_x++ )
		{
			const unsigned int texture_x= texel_x + lightmap_cell_x * monolighted_block_size;
			const unsigned int texture_y= texel_y + lightmap_cell_y * monolighted_block_size;
			const unsigned int texel_address= texture_x + texture_y * texture_size;
			const uint32_t texel= in_data[ texel_address ];
			unsigned char components[4];
			for( unsigned int i= 0u; i < 3u; i++ )
			{
				const unsigned int c= reinterpret_cast<const unsigned char*>(&texel)[i] * static_cast<unsigned int>(light) >> 16u;
				components[i]= std::min( c, 255u );
			}

			std::memcpy( &out_data[ texel_address ], components, sizeof(uint32_t) );
		}
	} // for lightmap cells

	return surface;
}

} // PanzerChasm
