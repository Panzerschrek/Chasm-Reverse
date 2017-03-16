#pragma once

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

private:
	void LoadModelsGroup( const std::vector<Model>& models, ModelsGroup& out_group );

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
};

} // PanzerChasm
