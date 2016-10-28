#pragma once
#include <memory>
#include <sstream>

#include <vec.hpp>

#include "assert.hpp"
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

	struct IndexElement
	{
		enum Type : unsigned short
		{
			None,
			StaticWall,
			DynamicWall,
			StaticModel,
			Item,
		};

		unsigned short type : 3;
		unsigned short index : 13;
	};

	SIZE_ASSERT( IndexElement, 2u );

	struct Procedure
	{
		float start_delay_s= 0.0f;
		float back_wait_s= 0.0f; // If zero - not reversible
		float speed= 0.0f;
		bool life_check= false;
		bool mortal= false;
		bool light_remap= false;
		bool locked= false;
		unsigned int loops= 0u;
		float loop_delay_s= 0.0f;
		unsigned int on_message_number= 0u;
		unsigned int first_message_number= 0u;
		unsigned int lock_message_number= 0u;
		unsigned int sfx_id= 0u;
		unsigned char sfx_pos[2];
		unsigned char link_switch_pos[2];

		bool red_key_required= false;
		bool green_key_required= false;
		bool blue_key_required= false;

		enum class ActionCommandId
		{
			Lock,
			Unlock,
			PlayAnimation,
			StopAnimation,
			Move,
			XMove,
			YMove,
			Rotate,
			Up,
			Light,

			Change,
			Death,
			Explode,
			Quake,
			Ambient,
			Wind,
			Source,

			Waitout,
			Nonstop,

			NumCommands,
			Unknown,
		};

		struct ActionCommand
		{
			ActionCommandId id;
			float args[8];
		};

		std::vector<ActionCommand> action_commands;
	};

	struct Message
	{
		float delay_s= 0.0f;

		struct Text
		{
			int x= 0, y= 0;
			std::string data;
		};
		std::vector<Text> texts;
	};

	struct Link
	{
		enum : unsigned char
		{
			None,
			Link_,
			Floor,
			Shoot,
			Return,
			Unlock,
			Destroy,
			OnOffLink,
		} type= None;
		unsigned char proc_id= 0u;
	};

public:
	std::vector<Wall> static_walls;
	std::vector<Wall> dynamic_walls;
	std::vector<Model> static_models;
	std::vector<Item> items;
	std::vector<Monster> monsters;

	std::vector<ModelDescription> models_description;

	std::vector<Message> messages;
	std::vector<Procedure> procedures;

	// All map tables cells are accesible via table[ x + y * size ].

	// [x; y] to element index in container (walls, items, etc.)
	IndexElement map_index[ c_map_size * c_map_size ];

	Link links[ c_map_size * c_map_size ];

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
	typedef std::array< bool, MapData::c_map_size * MapData::c_map_size > DynamicWallsMask;

private:
	void LoadLightmap( const Vfs::FileContent& map_file, MapData& map_data );
	void LoadWalls( const Vfs::FileContent& map_file, MapData& map_data, const DynamicWallsMask& dynamic_walls_mask );
	void LoadFloorsAndCeilings( const Vfs::FileContent& map_file, MapData& map_data );
	void LoadMonsters( const Vfs::FileContent& map_file, MapData& map_data );

	void LoadModelsDescription( const Vfs::FileContent& resource_file, MapData& map_data );
	void LoadWallsTexturesNames( const Vfs::FileContent& resource_file, MapData& map_data );

	void LoadFloorsTexturesData( const Vfs::FileContent& floors_file, MapData& map_data );

	void LoadLevelScripts( const Vfs::FileContent& process_file, MapData& map_data );

	void LoadMessage( unsigned int message_number, std::istringstream& stream, MapData& map_data );
	void LoadProcedure( unsigned int procedure_number, std::istringstream& stream, MapData& map_data );
	void LoadLinks( std::istringstream& stream, MapData& map_data );

	void MarkDynamicWalls( const MapData& map_data, DynamicWallsMask& out_dynamic_walls );

private:
	const VfsPtr vfs_;

	unsigned int last_loaded_map_number_= 0;
	MapDataConstPtr last_loaded_map_;
};

typedef std::shared_ptr<MapLoader> MapLoaderPtr;

} // namespace PanzerChasm
