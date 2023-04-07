#pragma once

#include "../map_loader.hpp"
#include "../model.hpp"
#include "../rendering_context.hpp"
#include "fwd.hpp"
#include "i_map_drawer.hpp"
#include "software_renderer/rasterizer.hpp"
#include "software_renderer/surfaces_cache.hpp"

namespace PanzerChasm
{

class MapDrawerSoft final : public IMapDrawer
{
public:
	MapDrawerSoft(
		Settings& settings,
		const GameResourcesConstPtr& game_resources,
		const RenderingContextSoft& rendering_context );
	virtual ~MapDrawerSoft() override;

	virtual void SetMap( const MapDataConstPtr& map_data ) override;

	virtual void Draw(
		const MapState& map_state,
		const m_Mat4& view_rotation_and_projection_matrix,
		const m_Vec3& camera_position,
		const ViewClipPlanes& view_clip_planes,
		EntityId player_monster_id ) override;

	virtual void DrawWeapon(
		const WeaponState& weapon_state,
		const m_Mat4& projection_matrix,
		const m_Vec3& camera_position,
		float x_angle, float z_angle,
		bool invisible ) override;

	virtual void DrawActiveItemIcon(
		const MapState& map_state,
		unsigned int icon_item_id,
		unsigned int slot_number ) override;

	virtual void DoFullscreenPostprocess( const MapState& map_state ) override;

	virtual void DrawMapRelatedModels(
		const MapRelatedModel* models, unsigned int model_count,
		const m_Mat4& view_rotation_and_projection_matrix,
		const m_Vec3& camera_position,
		const ViewClipPlanes& view_clip_planes ) override;

private:
	struct ModelsGroup
	{
		struct ModelEntry
		{
			unsigned int texture_size[2];
			unsigned int texture_data_offset; // in pixels
		};

		std::vector<ModelEntry> models;

		// TODO - add mips support
		std::vector<uint32_t> textures_data;
	};

	struct FloorCeilingCell
	{
		unsigned char xy[2];
		unsigned char texture_id;
		SurfacesCache::Surface* mips_surfaces[4];
	};

	struct DrawWall
	{
		unsigned int surface_width; // In pixels. must be 64 or 128
		unsigned char texture_id;
		unsigned char lightmap[8];

		SurfacesCache::Surface* mips_surfaces[4];
	};

	struct FloorTexture
	{
		// TODO - do not store mip0 32bit texture.
		uint32_t data[ MapData::c_floor_texture_size * MapData::c_floor_texture_size ];
		uint32_t mip1[ MapData::c_floor_texture_size * MapData::c_floor_texture_size /  4u ];
		uint32_t mip2[ MapData::c_floor_texture_size * MapData::c_floor_texture_size / 16u ];
		uint32_t mip3[ MapData::c_floor_texture_size * MapData::c_floor_texture_size / 64u ];
	};

	struct WallTexture
	{
		unsigned int size[2];

		unsigned char full_alpha_row[2];
		bool has_alpha; // Except low and bottom rejected rows.

		// TODO - do not store mip0 32bit texture.
		std::vector<uint32_t> data;
		uint32_t* mip0;
		uint32_t* mips[3]; // 1, 2, 3
	};

	struct SkyTexture
	{
		char file_name[32];
		unsigned int size[2];

		// TODO - add mips support.
		// TODO - do not store mip0 32bit texture.
		std::vector<uint32_t> data;
	};

	struct SpriteTexture
	{
		unsigned int size[3]; // Contains several frames

		// TODO - add mips support.
		// TODO - do not store mip0 32bit texture.
		std::vector<uint32_t> data;
	};

	struct TextureView
	{
		unsigned int size[2];
		const uint32_t* data;
	};

	struct PlayerTexture
	{
		unsigned int size[2];
		std::vector<uint32_t> data;
	};

private:
	void LoadModelsGroup( const std::vector<Model>& models, ModelsGroup& out_group );
	void LoadWallsTextures( const MapData& map_data );
	void LoadFloorsTextures( const MapData& map_data );
	void LoadWalls( const MapData& map_data );
	void LoadFloorsAndCeilings( const MapData& map_data );
	TextureView GetPlayerTexture( unsigned char color );

	template< bool is_dynamic_wall >
	void DrawWallSegment(
		DrawWall& wall,
		const m_Vec2& vert_pos0, const m_Vec2& vert_pos1, float z,
		float tc_0, float tc_1,
		const m_Mat4& matrix,
		const m_Vec2& camera_position_xy,
		const ViewClipPlanes& view_clip_planes );

	void DrawWalls( const MapState& map_state, const m_Mat4& matrix, const m_Vec2& camera_position_xy, const ViewClipPlanes& view_clip_planes );
	void DrawFloorsAndCeilings( const m_Mat4& matrix, const ViewClipPlanes& view_clip_planes  );

	void DrawModel(
		const ModelsGroup& models_group,
		const std::vector<Model>& model_group_models,
		unsigned int model_id,
		unsigned int animation_frame,
		const ViewClipPlanes& view_clip_planes,
		const m_Vec3& position,
		const m_Mat4& rotation_matrix,
		const m_Mat4& view_matrix,
		const m_Vec3& camera_position,
		unsigned char visible_groups_mask,
		bool transparent,
		bool force_transparent_nontransparent_polygons= false, // TODO - maybe make transparency-type enum?
		bool fullbright= false,
		unsigned int submodel_id= ~0u,  /* Submodel of model to draw. ~0 means base model. */
		unsigned char color= 0u /* For players only. */ );

	void DrawModelShadow(
		const Model& base_model,
		unsigned int animation_frame,
		const ViewClipPlanes& view_clip_planes,
		const m_Vec3& position,
		const m_Mat4& rotation_matrix,
		const m_Mat4& view_matrix,
		const m_Vec3& camera_position,
		const m_Vec3& light_pos,
		unsigned char visible_groups_mask,
		unsigned int submodel_id= ~0u  /* Submodel of model to draw. ~0 means base model. */ );

	void DrawSky(
		const m_Mat4& matrix,
		const m_Vec3& sky_pos,
		const ViewClipPlanes& view_clip_planes );

	void DrawEffectsSprites(
		const MapState& map_state,
		const m_Mat4& view_matrix,
		const m_Vec3& camera_position,
		const ViewClipPlanes& view_clip_planes );

	void DrawBMPObjectsSprites(
		const MapState& map_state,
		const m_Mat4& view_matrix,
		const m_Vec3& camera_position,
		const ViewClipPlanes& view_clip_planes );

	// Returns new vertex count.
	// clipped_vertices_ used
	unsigned int ClipPolygon(
		const m_Plane3& clip_plane,
		unsigned int vertex_count );

	template<unsigned int mip>
	const SurfacesCache::Surface* GetWallSurface( DrawWall& wall );

	template<unsigned int mip>
	const SurfacesCache::Surface* GetFloorCeilingSurface( FloorCeilingCell& cell );

private:
	struct ClippedVertex
	{
		m_Vec3 pos;
		m_Vec2 tc;
		ClippedVertex* next;
	};

private:
	Settings& settings_;
	const GameResourcesConstPtr game_resources_;
	const RenderingContextSoft rendering_context_;
	const float screen_transform_x_;
	const float screen_transform_y_;

	Rasterizer rasterizer_;
	SurfacesCache surfaces_cache_;

	MapDataConstPtr current_map_data_;
	std::unique_ptr<MapBSPTree> map_bsp_tree_;

	ModelsGroup map_models_;
	ModelsGroup items_models_;
	ModelsGroup rockets_models_;
	ModelsGroup gibs_models_;
	ModelsGroup weapons_models_;
	ModelsGroup monsters_models_;

	std::vector<DrawWall> static_walls_;
	std::vector<DrawWall> dynamic_walls_;

	std::vector<FloorCeilingCell> map_floors_and_ceilings_;
	unsigned int first_floor_= 0u;
	unsigned int first_ceiling_= 0u;

	std::vector<SpriteTexture> sprite_effects_textures_;
	std::vector<SpriteTexture> bmp_objects_sprites_;
	SkyTexture sky_texture_;

	std::vector<PlayerTexture> player_textures_;

	// Reuse vector (do not create new vector each frame).
	std::vector<const MapState::SpriteEffect*> sorted_sprites_;

	// Put large arrays at back.

	// Vertices for clipping.
	// Size of array must be not less, then ( max_vertices_in_polygon + 2 * max_clip_planes ).
	static constexpr unsigned int c_max_clip_vertices_= 32u;
	ClippedVertex clipped_vertices_[ c_max_clip_vertices_ ];
	ClippedVertex* fisrt_clipped_vertex_= nullptr;
	unsigned int next_new_clipped_vertex_= 0u;

	WallTexture wall_textures_[ MapData::c_max_walls_textures ];

	FloorTexture floor_textures_[ MapData::c_floors_textures_count ];
};

} // namespace PanzerChasm
