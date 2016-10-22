#include <cstring>
#include <sstream>

#include "math_utils.hpp"

#include "map_loader.hpp"

namespace PanzerChasm
{

namespace
{

constexpr unsigned int g_bytes_per_floor_in_file= 86u * MapData::c_floor_texture_size;
constexpr unsigned int g_floors_file_header_size= 64u;

constexpr float g_map_coords_scale= 1.0f / 256.0f;

} // namespace

#define SIZE_ASSERT(x, size) static_assert( sizeof(x) == size, "Invalid size" )

#pragma pack(push, 1)

struct MapWall
{
	unsigned char texture_id;
	unsigned char wall_size;
	unsigned char unknown;
	unsigned short vert_coord[2][2];
};

SIZE_ASSERT( MapWall, 11 );

struct MapLight
{
	unsigned short position[2u];
	unsigned short r0;
	unsigned short r1;
	unsigned char unknown[4u];
};

SIZE_ASSERT( MapLight, 12 );

struct MapMonster
{
	unsigned short position[2];
	unsigned short id;
	unsigned char angle; // bits 0-3 is angle. Other bits - unknown.
	unsigned char unknown;
};

SIZE_ASSERT( MapMonster, 8 );

#pragma pack(pop)

MapLoader::MapLoader( const VfsPtr& vfs )
	: vfs_(vfs)
{}

MapLoader::~MapLoader()
{}

MapDataConstPtr MapLoader::LoadMap( const unsigned int map_number )
{
	if( map_number == last_loaded_map_number_ && last_loaded_map_ != nullptr )
		return last_loaded_map_;

	char map_file_name[16];
	char resource_file_name[16];
	char floors_file_name[16];

	std::snprintf( map_file_name, sizeof(map_file_name), "MAP.%02u", map_number );
	std::snprintf( resource_file_name, sizeof(resource_file_name), "RESOURCE.%02u", map_number );
	std::snprintf( floors_file_name, sizeof(floors_file_name), "FLOORS.%02u", map_number );

	const Vfs::FileContent map_file_content= vfs_->ReadFile( map_file_name );
	const Vfs::FileContent resource_file_content= vfs_->ReadFile( resource_file_name );
	const Vfs::FileContent floors_file_content= vfs_->ReadFile( floors_file_name );

	if( map_file_content.empty() ||
		resource_file_content.empty() ||
		floors_file_content.empty() )
	{
		return nullptr;
	}

	MapDataPtr result= std::make_shared<MapData>();

	// Scan map file
	LoadLightmap( map_file_content,*result );
	LoadWalls( map_file_content, *result );
	LoadFloorsAndCeilings( map_file_content,*result );
	LoadMonsters( map_file_content, *result );

	// Scan resource file
	LoadModelsDescription( resource_file_content, *result );
	LoadWallsTexturesNames( resource_file_content, *result );

	// Scan floors file
	LoadFloorsTexturesData( floors_file_content, *result );

	// Cache result and return it
	last_loaded_map_number_= map_number;
	last_loaded_map_= result;
	return result;
}

void MapLoader::LoadLightmap( const Vfs::FileContent& map_file, MapData& map_data )
{
	const unsigned int c_lightmap_data_offset= 0x01u;

	const unsigned char* const in_data= map_file.data() + c_lightmap_data_offset;

	// TODO - tune formula
	for( unsigned int y= 0u; y < MapData::c_lightmap_size; y++ )
	for( unsigned int x= 0u; x < MapData::c_lightmap_size - 1u; x++ )
	{
		map_data.lightmap[ x + 1u + y * MapData::c_lightmap_size ]=
			255u - ( ( in_data[ x + y * MapData::c_lightmap_size ] - 3u ) * 6u );
	}
}

void MapLoader::LoadWalls( const Vfs::FileContent& map_file, MapData& map_data )
{
	const unsigned int c_walls_offset= 0x18001u;

	for( unsigned int y= 0u; y < MapData::c_map_size; y++ )
	for( unsigned int x= 1u; x < MapData::c_map_size; x++ )
	{
		const MapWall& map_wall=
			*reinterpret_cast<const MapWall*>( map_file.data() + c_walls_offset + sizeof(MapWall) * ( x + y * MapData::c_map_size ) );

		if( map_wall.texture_id >= 128u )
		{
			const unsigned int c_first_item= 131u;
			const unsigned int c_first_model= 163u;

			if( map_wall.texture_id >= c_first_model )
			{
				map_data.static_models.emplace_back();
				MapData::Model& model= map_data.static_models.back();
				model.pos.x= float(map_wall.vert_coord[0][0]) * g_map_coords_scale;
				model.pos.y= float(map_wall.vert_coord[0][1]) * g_map_coords_scale;
				model.angle= float(map_wall.unknown & 7u) / 8.0f * Constants::two_pi + Constants::pi;
				model.model_id= map_wall.texture_id - c_first_model;
			}
			else if( map_wall.texture_id >= c_first_item )
			{
				map_data.items.emplace_back();
				MapData::Item& model= map_data.items.back();
				model.pos.x= float(map_wall.vert_coord[0][0]) * g_map_coords_scale;
				model.pos.y= float(map_wall.vert_coord[0][1]) * g_map_coords_scale;
				model.angle= float(map_wall.unknown & 7u) / 8.0f * Constants::two_pi + Constants::pi;
				model.item_id=  map_wall.texture_id - c_first_item;
			}
			continue;
		}

		if( !( map_wall.wall_size == 64u || map_wall.wall_size == 128u ) )
			continue;

		map_data.static_walls.emplace_back();
		MapData::Wall& wall= map_data.static_walls.back();

		wall.vert_pos[0].x= float(map_wall.vert_coord[0][0]) * g_map_coords_scale;
		wall.vert_pos[0].y= float(map_wall.vert_coord[0][1]) * g_map_coords_scale;
		wall.vert_pos[1].x= float(map_wall.vert_coord[1][0]) * g_map_coords_scale;
		wall.vert_pos[1].y= float(map_wall.vert_coord[1][1]) * g_map_coords_scale;

		wall.vert_tex_coord[0]= 0.0f;
		wall.vert_tex_coord[1]= float(map_wall.wall_size) / 128.0f;

		wall.texture_id= map_wall.texture_id;
	} // for xy
}

void MapLoader::LoadFloorsAndCeilings( const Vfs::FileContent& map_file, MapData& map_data )
{
	const unsigned int c_offset= 0x23001u;

	for( unsigned int floor_or_ceiling= 0u; floor_or_ceiling < 2u; floor_or_ceiling++ )
	{
		const unsigned char* const in_data=
			map_file.data() + c_offset + MapData::c_map_size * MapData::c_map_size * floor_or_ceiling;

		unsigned char* const out_data=
			floor_or_ceiling == 0u
				? map_data.floor_textures
				: map_data.ceiling_textures;

		for( unsigned int x= 0u; x < MapData::c_map_size; x++ )
		for( unsigned int y= 0u; y < MapData::c_map_size; y++ )
		{
			const unsigned char texture_number= in_data[ x * MapData::c_map_size + y ];
			out_data[ x + y * MapData::c_map_size ]= texture_number;
		}
	}
}

void MapLoader::LoadMonsters( const Vfs::FileContent& map_file, MapData& map_data )
{
	const unsigned int c_lights_count_offset= 0x27001u;
	const unsigned int c_lights_offset= 0x27003u;
	unsigned short lights_count;
	std::memcpy( &lights_count, map_file.data() + c_lights_count_offset, sizeof(unsigned short) );

	const unsigned int monsters_count_offset= c_lights_offset + sizeof(MapLight) * lights_count;
	const unsigned int monsters_offset= monsters_count_offset + 2u;
	const MapMonster* map_monsters= reinterpret_cast<const MapMonster*>( map_file.data() + monsters_offset );
	unsigned short monster_count;
	std::memcpy( &monster_count, map_file.data() + monsters_count_offset, sizeof(unsigned short) );

	map_data.monsters.resize( monster_count );
	for( unsigned int m= 0u; m < monster_count; m++ )
	{
		MapData::Monster& monster= map_data.monsters[m];

		monster.pos.x= float(map_monsters[m].position[0]) * g_map_coords_scale;
		monster.pos.y= float(map_monsters[m].position[1]) * g_map_coords_scale;
		monster.monster_id= map_monsters[m].id - 100u;
		monster.angle=
			float( map_monsters[m].angle & 7u) / 8.0f * Constants::two_pi +
			1.5f * Constants::pi;
	}
}

void MapLoader::LoadModelsDescription( const Vfs::FileContent& resource_file, MapData& map_data )
{
	const char* start= std::strstr( reinterpret_cast<const char*>( resource_file.data() ), "#newobjects" );

	while( *start != '\n' ) start++;
	start++;

	const char* const end= std::strstr( start, "#end" );

	std::istringstream stream( std::string( start, end ) );

	while( !stream.eof() )
	{
		char line[ 512u ];
		stream.getline( line, sizeof(line), '\n' );

		if( stream.eof() )
			break;

		std::istringstream line_stream{ std::string( line ) };

		double num;
		line_stream >> num; // GoRad
		line_stream >> num; // Shad
		line_stream >> num; // BObj
		line_stream >> num; // BMPz
		line_stream >> num; // AC
		line_stream >> num; // Blw
		line_stream >> num; // BLmt
		line_stream >> num; // SFX
		line_stream >> num; // BSfx

		map_data.models_description.emplace_back();
		MapData::ModelDescription& model_description= map_data.models_description.back();

		line_stream >> model_description.file_name; // FileName

		model_description.animation_file_name[0u]= '\0';
		line_stream >> model_description.animation_file_name;
	}
}

void MapLoader::LoadWallsTexturesNames( const Vfs::FileContent& resource_file, MapData& map_data )
{
	for( char* const file_name : map_data.walls_textures )
		file_name[0]= '\0';

	const char* start= std::strstr( reinterpret_cast<const char*>( resource_file.data() ), "#GFX" );
	start+= std::strlen( "#GFX" );
	const char* const end= std::strstr( start, "#end" );

	std::istringstream stream( std::string( start, end ) );

	char line[ 512u ];
	stream.getline( line, sizeof(line), '\n' );

	while( !stream.eof() )
	{
		stream.getline( line, sizeof(line), '\n' );

		if( stream.eof() )
			break;

		std::istringstream line_stream{ std::string( line ) };

		unsigned int texture_number;
		line_stream >> texture_number;

		char colon[8];
		line_stream >> colon;

		line_stream >> map_data.walls_textures[ texture_number ];
	}
}

void MapLoader::LoadFloorsTexturesData( const Vfs::FileContent& floors_file, MapData& map_data )
{
	for( unsigned int t= 0u; t < MapData::c_floors_textures_count; t++ )
	{
		const unsigned char* const in_data=
			floors_file.data() + g_floors_file_header_size + g_bytes_per_floor_in_file * t;

		std::memcpy(
			map_data.floor_textures_data[t],
			in_data,
			MapData::c_floor_texture_size * MapData::c_floor_texture_size );
	}
}

} // namespace PanzerChasm
