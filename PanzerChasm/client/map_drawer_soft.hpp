#pragma once

#include "../map_loader.hpp"
#include "../model.hpp"
#include "../rendering_context.hpp"
#include "i_map_drawer.hpp"
#include "software_renderer/rasterizer.hpp"

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
		EntityId player_monster_id ) override;

	virtual void DrawWeapon(
		const WeaponState& weapon_state,
		const m_Mat4& projection_matrix,
		const m_Vec3& camera_position,
		float x_angle, float z_angle ) override;

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
	};

	struct FloorTexture
	{
		// TODO - add mips support.
		// TODO - do not store mip0 32bit texture.
		uint32_t data[ MapData::c_floor_texture_size * MapData::c_floor_texture_size ];
	};

	struct WallTexture
	{
		unsigned int size[2];
		// TODO - add mips support.
		// TODO - do not store mip0 32bit texture.
		std::vector<uint32_t> data;
	};

private:
	void LoadModelsGroup( const std::vector<Model>& models, ModelsGroup& out_group );
	void LoadWallsTextures( const MapData& map_data );
	void LoadFloorsTextures( const MapData& map_data );
	void LoadFloorsAndCeilings( const MapData& map_data);

	void DrawWalls( const m_Mat4& matrix );
	void DrawFloorsAndCeilings( const m_Mat4& matrix );

	void DrawModel(
		const m_Mat4& matrix,
		const ModelsGroup& models_group,
		const std::vector<Model>& models_group_models,
		unsigned int model_id,
		unsigned int animation_frame );

private:
	const GameResourcesConstPtr game_resources_;
	const RenderingContextSoft rendering_context_;

	Rasterizer rasterizer_;

	MapDataConstPtr current_map_data_;

	ModelsGroup map_models_;
	ModelsGroup items_models_;
	ModelsGroup rockets_models_;
	ModelsGroup weapons_models_;
	ModelsGroup monsters_models_;

	std::vector<FloorCeilingCell> map_floors_and_ceilings_;
	unsigned int first_floor_= 0u;
	unsigned int first_ceiling_= 0u;

	// Put large arrays at back.

	WallTexture wall_textures_[ MapData::c_max_walls_textures ];

	FloorTexture floor_textures_[ MapData::c_floors_textures_count ];
};

} // namespace PanzerChasm
