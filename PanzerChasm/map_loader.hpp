#pragma once
#include <memory>

#include "vec.hpp"

#include "vfs.hpp"

namespace PanzerChasm
{

struct MapData
{
public:
	static constexpr unsigned int c_map_size_log2= 6u;
	static constexpr unsigned int c_map_size= 1u << c_map_size_log2;

	static constexpr unsigned int c_floor_texture_size_log2= 6u;
	static constexpr unsigned int c_floor_texture_size= 1u << c_floor_texture_size_log2;

	static constexpr unsigned int c_lightmap_scale= 4u;
	static constexpr unsigned int c_lightmap_size= c_lightmap_scale * c_map_size;

	static constexpr unsigned int c_max_walls_textures= 128u;
	static constexpr unsigned int c_floors_textures_count= 64u;

	static constexpr unsigned int c_max_file_name_size= 16u;

public:
	struct Wall
	{
		m_Vec2 vert_pos[2];
		float vert_tex_coord[2];
		unsigned char texture_id;
	};

	struct Model
	{
		m_Vec2 pos;
		float angle;
		unsigned char model_id;
	};

	struct Item
	{
		m_Vec2 pos;
		float angle;
		unsigned char item_id;
	};

	struct Monster
	{
		m_Vec2 pos;
		float angle;
		unsigned char monster_id;
	};

	struct ModelDescription
	{
		char file_name[ c_max_file_name_size ];
		char animation_file_name[ c_max_file_name_size ]; // May be empty
	};

public:
	std::vector<Wall> static_walls;
	std::vector<Wall> dynamic_walls;
	std::vector<Model> static_models;
	std::vector<Item> items;
	std::vector<Monster> monsters;

	std::vector<ModelDescription> models_description;

	// All map tables cells are accesible via table[ x + y * size ].

	char walls_textures[ c_max_walls_textures ][ c_max_file_name_size ];

	unsigned char floor_textures[ c_map_size * c_map_size ];
	unsigned char ceiling_textures[ c_map_size * c_map_size ];

	unsigned char lightmap[ c_lightmap_size * c_lightmap_size ];

	unsigned char floor_textures_data[ c_floors_textures_count ][ c_floor_texture_size * c_floor_texture_size ];
};

typedef std::shared_ptr<MapData> MapDataPtr;
typedef std::shared_ptr<const MapData> MapDataConstPtr;

class MapLoader final
{
public:
	explicit MapLoader( const VfsPtr& vfs );
	~MapLoader();

	MapDataConstPtr LoadMap( unsigned int map_number );

private:
	void LoadLightmap( const Vfs::FileContent& map_file, MapData& map_data );
	void LoadWalls( const Vfs::FileContent& map_file, MapData& map_data );
	void LoadFloorsAndCeilings( const Vfs::FileContent& map_file, MapData& map_data );
	void LoadMonsters( const Vfs::FileContent& map_file, MapData& map_data );

	void LoadModelsDescription( const Vfs::FileContent& resource_file, MapData& map_data );
	void LoadWallsTexturesNames( const Vfs::FileContent& resource_file, MapData& map_data );

	void LoadFloorsTexturesData( const Vfs::FileContent& floors_file, MapData& map_data );

private:
	const VfsPtr vfs_;

	unsigned int last_loaded_map_number_= 0;
	MapDataConstPtr last_loaded_map_;
};

typedef std::shared_ptr<MapLoader> MapLoaderPtr;

} // namespace PanzerChasm
