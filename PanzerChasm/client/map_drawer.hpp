#pragma once

#include <framebuffer.hpp>
#include <glsl_program.hpp>
#include <polygon_buffer.hpp>
#include <texture.hpp>

#include "../fwd.hpp"
#include "../rendering_context.hpp"
#include "fwd.hpp"
#include "map_state.hpp"

namespace PanzerChasm
{

struct WallVertex;

class MapDrawer final
{
public:
	MapDrawer(
		GameResourcesConstPtr game_resources,
		const RenderingContext& rendering_context );
	~MapDrawer();

	void SetMap( const MapDataConstPtr& map_data );

	void Draw(
		const MapState& map_state,
		const m_Mat4& view_rotation_and_projection_matrix,
		const m_Vec3& camera_position,
		EntityId player_monster_id );

	void DrawWeapon(
		const WeaponState& weapon_state,
		const m_Mat4& projection_matrix,
		const m_Vec3& camera_position );

private:
	struct FloorGeometryInfo
	{
		unsigned int first_vertex_number;
		unsigned int vertex_count;
	};

	struct ModelGeometry
	{
		unsigned int frame_count;
		unsigned int vertex_count;
		unsigned int first_vertex_index;
		unsigned int first_index;
		unsigned int index_count;
		unsigned int first_transparent_index;
		unsigned int transparent_index_count;
	};

	struct MonsterModel
	{
		ModelGeometry geometry_description;
		ModelGeometry submodels_geometry_description[3];

		r_Texture texture;
	};

private:
	void LoadSprites( const std::vector<ObjSprite>& sprites, std::vector<GLuint>& out_textures );
	void PrepareSkyGeometry();

	void LoadFloorsTextures( const MapData& map_data );
	void LoadWallsTextures( const MapData& map_data );

	void LoadFloors( const MapData& map_data );
	void LoadWalls( const MapData& map_data );

	void LoadModels(
		const std::vector<Model>& models,
		std::vector<ModelGeometry>& out_geometry,
		r_PolygonBuffer& out_geometry_data,
		GLuint& out_textures_array ) const;

	void LoadMonstersModels();

	void UpdateDynamicWalls( const MapState::DynamicWalls& dynamic_walls );

	static void PrepareModelsPolygonBuffer(
		std::vector<Model::Vertex>& vertices,
		std::vector<unsigned short>& indeces,
		r_PolygonBuffer& buffer );

	void DrawWalls( const m_Mat4& view_matrix );
	void DrawFloors( const m_Mat4& view_matrix );

	void DrawModels(
		const MapState& map_state,
		const m_Mat4& view_matrix,
		bool transparent );

	void DrawItems(
		const MapState& map_state,
		const m_Mat4& view_matrix,
		bool transparent );

	void DrawDynamicItems(
		const MapState& map_state,
		const m_Mat4& view_matrix,
		bool transparent );

	void DrawMonsters(
		const MapState& map_state,
		const m_Mat4& view_matrix,
		EntityId player_mosnter_id,
		bool transparent );

	void DrawMonstersBodyParts(
		const MapState& map_state,
		const m_Mat4& view_matrix,
		bool transparent );

	void DrawRockets(
		const MapState& map_state,
		const m_Mat4& view_matrix,
		bool transparent );

	void DrawBMPObjectsSprites(
		const MapState& map_state,
		const m_Mat4& view_matrix,
		const m_Vec3& camera_position );

	void DrawEffectsSprites(
		const MapState& map_state,
		const m_Mat4& view_matrix,
		const m_Vec3& camera_position );

	void DrawSky( const m_Mat4& view_rotation_and_projection_matrix );

private:
	const GameResourcesConstPtr game_resources_;
	const RenderingContext rendering_context_;

	MapDataConstPtr current_map_data_;

	r_Texture lightmap_;
	r_Texture* active_lightmap_= nullptr; // Build-in or hd lightmap
	r_Texture fullbright_lightmap_dummy_;

	GLuint floor_textures_array_id_= ~0;
	GLuint wall_textures_array_id_= ~0;
	GLuint models_textures_array_id_= ~0;
	GLuint items_textures_array_id_= ~0;
	GLuint rockets_textures_array_id_= ~0;
	GLuint weapons_textures_array_id_= ~0;

	std::vector<GLuint> sprites_textures_arrays_;
	std::vector<GLuint> bmp_objects_sprites_textures_arrays_;

	r_GLSLProgram floors_shader_;
	r_PolygonBuffer floors_geometry_;
	// 0 - floor, 1 - ceiling
	FloorGeometryInfo floors_geometry_info[2];

	r_GLSLProgram walls_shader_;
	r_PolygonBuffer walls_geometry_;

	r_PolygonBuffer dynamic_walls_geometry_;
	std::vector<WallVertex> dynamc_walls_vertices_;

	r_GLSLProgram models_shader_;

	std::vector<ModelGeometry> models_geometry_;
	r_PolygonBuffer models_geometry_data_;

	std::vector<ModelGeometry> items_geometry_;
	r_PolygonBuffer items_geometry_data_;

	std::vector<ModelGeometry> rockets_geometry_;
	r_PolygonBuffer rockets_geometry_data_;

	std::vector<ModelGeometry> weapons_geometry_;
	r_PolygonBuffer weapons_geometry_data_;

	r_GLSLProgram sprites_shader_;

	r_GLSLProgram monsters_shader_;
	std::vector<MonsterModel> monsters_models_;
	r_PolygonBuffer monsters_geometry_data_;

	char current_sky_texture_file_name_[32];
	r_GLSLProgram sky_shader_;
	r_Texture sky_texture_;
	r_PolygonBuffer sky_geometry_data_;

	// Reuse vector (do not create new vector each frame).
	std::vector<const MapState::SpriteEffect*> sorted_sprites_;
};

} // PanzerChasm
