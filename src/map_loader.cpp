#include <cctype>
#include <cstring>

#include "assert.hpp"
#include "log.hpp"
#include "math_utils.hpp"
#include "common/files.hpp"
#include "map_loader.hpp"

namespace PanzerChasm
{

namespace
{

const unsigned int g_max_map_number= 30u;

constexpr unsigned int g_bytes_per_floor_in_file= 86u * MapData::c_floor_texture_size;
constexpr unsigned int g_floors_file_header_size= 64u;

constexpr float g_map_coords_scale= 1.0f / 256.0f;

} // namespace

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
	unsigned short light_power; // x4 value from editor.
	unsigned short r0; // x4 inner radius from editor.
	unsigned short r1; // x4 outer radius from editor.
	unsigned short bm; // x4 bm from editor + 32.
};

SIZE_ASSERT( MapLight, 12 );

struct MapMonster
{
	unsigned short position[2];
	unsigned short id;
	unsigned char angle; // bits 0-3 is angle. Other bits - unknown.
	unsigned char difficulty_flags;
};

SIZE_ASSERT( MapMonster, 8 );

#pragma pack(pop)

namespace
{

static decltype(MapData::Link::type) LinkTypeFromString( const char* const str )
{
	if( StringEquals( str, "link" ) )
		return MapData::Link::Link_;
	if( StringEquals( str, "floor" ) )
		return MapData::Link::Floor;
	if( StringEquals( str, "shoot" ) )
		return MapData::Link::Shoot;
	if( StringEquals( str, "return" ) )
		return MapData::Link::Return;
	if( StringEquals( str, "returnf" ) )
		return MapData::Link::ReturnFloor;
	if(StringEquals( str, "destroy" ) )
		return MapData::Link::Destroy;

	return MapData::Link::None;
}

MapData::Procedure::ActionCommandId ActionCommandFormString( const char* const str )
{
	using Command= MapData::Procedure::ActionCommandId;

	if( StringEquals( str, "lock" ) )
		return Command::Lock;
	if( StringEquals( str, "unlock" ) )
		return Command::Unlock;
	if( StringEquals( str, "playani" ) )
		return Command::PlayAnimation;
	if( StringEquals( str, "stopani" ) )
		return Command::StopAnimation;
	if( StringEquals( str, "move" ) )
		return Command::Move;
	if( StringEquals( str, "xmove" ) )
		return Command::XMove;
	if( StringEquals( str, "ymove" ) )
		return Command::YMove;
	if( StringEquals( str, "rotate" ) )
		return Command::Rotate;
	if( StringEquals( str, "up" ) )
		return Command::Up;
	if( StringEquals( str, "light" ) )
		return Command::Light;
	if( StringEquals( str, "change" ) )
		return Command::Change;
	if( StringEquals( str, "death" ) )
		return Command::Death;
	if( StringEquals( str, "explode" ) )
		return Command::Explode;
	if( StringEquals( str, "quake" ) )
		return Command::Quake;
	if( StringEquals( str, "ambient" ) )
		return Command::Ambient;
	if( StringEquals( str, "wind" ) )
		return Command::Wind;
	if( StringEquals( str, "source" ) )
		return Command::Source;
	if( StringEquals( str, "waitout" ) )
		return Command::Waitout;
	if( StringEquals( str, "nonstop" ) )
		return Command::Nonstop;

	return Command::Unknown;
}

static unsigned char ConvertMapLight( const unsigned char map_light )
{
	return 255u - map_light * 6u;
}

} // namespace

MapLoader::MapLoader( const VfsPtr& vfs )
	: vfs_(vfs)
{}

MapLoader::~MapLoader()
{}

MapDataConstPtr MapLoader::LoadMap( const unsigned int map_number )
{
	if( map_number >= 100 )
		return nullptr;

	if( last_loaded_map_ != nullptr && last_loaded_map_->number == map_number )
		return last_loaded_map_;

	Log::Info( "Loading map ", map_number );

	char level_path[ MapData::c_max_file_path_size ];
	char map_file_name[ MapData::c_max_file_path_size ];
	char resource_file_name[ MapData::c_max_file_path_size ];
	char floors_file_name[ MapData::c_max_file_path_size ];
	char process_file_name[ MapData::c_max_file_path_size ];

	std::snprintf( level_path, sizeof(level_path), "LEVEL%02u/", map_number );
	std::snprintf( map_file_name, sizeof(map_file_name), "%sMAP.%02u", level_path, map_number );
	std::snprintf( resource_file_name, sizeof(resource_file_name), "%sRESOURCE.%02u", level_path, map_number );
	std::snprintf( floors_file_name, sizeof(floors_file_name), "%sFLOORS.%02u", level_path, map_number );
	std::snprintf( process_file_name, sizeof(process_file_name), "%sPROCESS.%02u", level_path, map_number );

	const Vfs::FileContent map_file_content= vfs_->ReadFile( map_file_name );
	const Vfs::FileContent resource_file_content= vfs_->ReadFile( resource_file_name );
	const Vfs::FileContent floors_file_content= vfs_->ReadFile( floors_file_name );
	const Vfs::FileContent process_file_content= vfs_->ReadFile( process_file_name );

	if( map_file_content.empty() ||
		resource_file_content.empty() ||
		floors_file_content.empty() ||
		process_file_content.empty() )
	{
		Log::Warning( "Map ", map_number, " not found" );
		return nullptr;
	}

	std::snprintf( textures_path_, sizeof(textures_path_), "%sGFX/", level_path );
	std::snprintf( models_path_, sizeof(models_path_), "%s3D/", level_path );
	std::snprintf( animations_path_, sizeof(animations_path_), "%sANI/", level_path );

	MapDataPtr result= std::make_shared<MapData>();

	LoadLevelScripts( process_file_content, *result );

	DynamicWallsMask dynamic_walls_mask;
	MarkDynamicWalls( *result, dynamic_walls_mask );

	for( MapData::IndexElement & el : result->map_index )
		el.type= MapData::IndexElement::None;

	// Scan map file
	LoadLightmap( map_file_content, *result );
	const unsigned char* const walls_lightmaps_data= GetWallsLightmapData( map_file_content );
	LoadWalls( map_file_content, *result, dynamic_walls_mask, walls_lightmaps_data );
	LoadFloorsAndCeilings( map_file_content,*result );
	LoadAmbientLight( map_file_content,*result );
	LoadAmbientSoundsMap( map_file_content,*result );
	LoadMonstersAndLights( map_file_content, *result );

	// Scan resource file
	LoadMapName( resource_file_content, result->map_name );
	LoadSkyTextureName( resource_file_content, *result );
	LoadModelsDescription( resource_file_content, *result );
	LoadWallsTexturesDescription( resource_file_content, *result );
	LoadSoundsDescriptionFromMapResourcesFile( resource_file_content, result->map_sounds, MapData::c_max_map_sounds );
	LoadAmbientSoundsDescriptionFromMapResourcesFile( resource_file_content, result->ambients, MapData::c_max_map_ambients );

	// Scan floors file
	LoadFloorsTexturesData( floors_file_content, *result );

	LoadModels( *result );

	// Cache result and return it.
	result->number= map_number;
	last_loaded_map_= result;
	return result;
}

MapLoader::MapInfo MapLoader::GetNextMapInfo( unsigned int map_number )
{
	MapInfo result;

	// TODO - check if there are no maps?
	do
	{
		map_number++;
		if( map_number > g_max_map_number )
			map_number= 1u;
	}
	while( !GetMapInfoImpl( map_number, result ) );

	return result;
}

MapLoader::MapInfo MapLoader::GetPrevMapInfo( unsigned int map_number )
{
	MapInfo result;

	// TODO - check if there are no maps?
	do
	{
		if( map_number <= 1u )
			map_number= g_max_map_number;
		else
			map_number--;
	}
	while( !GetMapInfoImpl( map_number, result ) );

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
			ConvertMapLight( in_data[ x + y * MapData::c_lightmap_size ] );
	}
}

const unsigned char* MapLoader::GetWallsLightmapData( const Vfs::FileContent& map_file )
{
	const unsigned int c_walls_lightmap_data_offset= 0x01u + MapData::c_lightmap_size * MapData::c_lightmap_size;
	return map_file.data() + c_walls_lightmap_data_offset;
}

void MapLoader::LoadWalls(
	const Vfs::FileContent& map_file,
	MapData& map_data,
	const DynamicWallsMask& dynamic_walls_mask,
	const unsigned char* walls_lightmap_data )
{
	const unsigned int c_walls_offset= 0x18001u;

	for( unsigned int y= 0u; y < MapData::c_map_size; y++ )
	for( unsigned int x= 0u; x < MapData::c_map_size; x++ )
	{
		const bool is_dynamic= dynamic_walls_mask[ x + y * MapData::c_map_size ];
		MapData::IndexElement& index_element= map_data.map_index[ x + y * MapData::c_map_size ];

		const MapWall& map_wall=
			*reinterpret_cast<const MapWall*>( map_file.data() + c_walls_offset + sizeof(MapWall) * ( y + x * MapData::c_map_size ) );

		if( map_wall.texture_id == 0u )
		{
			// Trash
			continue;
		}
		if( map_wall.texture_id >= 128u )
		{
			const unsigned int c_first_item= 131u;
			const unsigned int c_first_model= 163u;

			if( map_wall.texture_id == 250u )
			{
				// Many walls have thist texture id. I don`t know, what is that. Lights, maybe?
			}
			else if( map_wall.texture_id >= c_first_model + 64u )
			{
				// Presumably, levels have no more, then 64 models.
			}
			else if( map_wall.texture_id >= c_first_model )
			{
				// Presumably, this is mask of difficulty.
				// bit 0 - easy, 1 - normal, 2 - hard, 3 - deathmatch
				const unsigned int difficulty_levels_mask= map_wall.vert_coord[1][0] & 7u;
				PC_UNUSED( difficulty_levels_mask );

				map_data.static_models.emplace_back();
				MapData::StaticModel& model= map_data.static_models.back();
				model.pos.x= float(map_wall.vert_coord[0][0]) * g_map_coords_scale;
				model.pos.y= float(map_wall.vert_coord[0][1]) * g_map_coords_scale;
				model.angle= float(map_wall.unknown & 7u) / 8.0f * Constants::two_pi + Constants::pi;
				model.model_id= map_wall.texture_id - c_first_model;
				model.difficulty_flags= map_wall.vert_coord[1][0];
				model.is_dynamic= is_dynamic;

				index_element.type= MapData::IndexElement::StaticModel;
				index_element.index= map_data.static_models.size() - 1u;
			}
			else if( map_wall.texture_id >= c_first_item )
			{
				map_data.items.emplace_back();
				MapData::Item& item= map_data.items.back();
				item.pos.x= float(map_wall.vert_coord[0][0]) * g_map_coords_scale;
				item.pos.y= float(map_wall.vert_coord[0][1]) * g_map_coords_scale;
				item.angle= float(map_wall.unknown & 7u) / 8.0f * Constants::two_pi + Constants::pi;
				item.item_id=  map_wall.texture_id - c_first_item;
				item.difficulty_flags= map_wall.vert_coord[1][0];

				index_element.type= MapData::IndexElement::Item;
				index_element.index= map_data.items.size() - 1u;
			}
			continue;
		}

		if( !( map_wall.wall_size == 64u || map_wall.wall_size == 128u ) )
			continue;

		auto& walls_container= is_dynamic ? map_data.dynamic_walls : map_data.static_walls;
		walls_container.emplace_back();
		MapData::Wall& wall= walls_container.back();

		index_element.type= is_dynamic ? MapData::IndexElement::DynamicWall : MapData::IndexElement::StaticWall;
		index_element.index= walls_container.size() - 1u;

		wall.vert_pos[0].x= float(map_wall.vert_coord[0][0]) * g_map_coords_scale;
		wall.vert_pos[0].y= float(map_wall.vert_coord[0][1]) * g_map_coords_scale;
		wall.vert_pos[1].x= float(map_wall.vert_coord[1][0]) * g_map_coords_scale;
		wall.vert_pos[1].y= float(map_wall.vert_coord[1][1]) * g_map_coords_scale;

		wall.vert_tex_coord[0]= float(map_wall.wall_size) / 128.0f;
		wall.vert_tex_coord[1]= 0.0f;

		wall.texture_id= map_wall.texture_id;

		// TODO - know, why "-2". Correct if needed.
		const unsigned char* const in_lightmap_data= walls_lightmap_data + ( x + y * MapData::c_map_size ) * 8u - 2u;
		for( unsigned int i= 0u; i < 8u; i++ )
			wall.lightmap[i]= ConvertMapLight( in_lightmap_data[i] );
	} // for xy
}

void MapLoader::LoadFloorsAndCeilings( const Vfs::FileContent& map_file, MapData& map_data )
{
	const unsigned int c_offset= 0x23001u;

	for( int floor_or_ceiling= 1u; floor_or_ceiling >= 0; floor_or_ceiling-- )
	{
		const unsigned char* const in_data=
			map_file.data() + c_offset + MapData::c_map_size * MapData::c_map_size * static_cast<unsigned int>(1 - floor_or_ceiling);

		unsigned char* const out_data=
			floor_or_ceiling == 0
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

void MapLoader::LoadAmbientLight( const Vfs::FileContent& map_file, MapData& map_data )
{
	const unsigned int c_ambient_lightmap_offset= 0x23001u + MapData::c_map_size * MapData::c_map_size * 2u;

	const unsigned char* in_ambient_lightmap_data= map_file.data() + c_ambient_lightmap_offset;
	for( unsigned int x= 0u; x < MapData::c_map_size; x++ )
	for( unsigned int y= 0u; y < MapData::c_map_size; y++ )
	{
		// Convert "darkeness" into light.
		const int l= std::max( 18 - int( in_ambient_lightmap_data[ x * MapData::c_map_size + y ] ), 0 ) * 6;
		map_data.ambient_lightmap[ x + y * MapData::c_map_size ]= static_cast<unsigned char>(l);
	}
}

void MapLoader::LoadAmbientSoundsMap( const Vfs::FileContent& map_file, MapData& map_data )
{
	const unsigned int c_offset= 0x23001u + MapData::c_map_size * MapData::c_map_size * 3u;

	const unsigned char* in_data= map_file.data() + c_offset;
	for( unsigned int x= 0u; x < MapData::c_map_size; x++ )
	for( unsigned int y= 0u; y < MapData::c_map_size; y++ )
		map_data.ambient_sounds_map[ x + y * MapData::c_map_size ]= in_data[ x * MapData::c_map_size + y ];
}

void MapLoader::LoadMonstersAndLights( const Vfs::FileContent& map_file, MapData& map_data )
{
	const unsigned int c_lights_count_offset= 0x27001u;
	const unsigned int c_lights_offset= 0x27003u;
	unsigned short lights_count;
	std::memcpy( &lights_count, map_file.data() + c_lights_count_offset, sizeof(unsigned short) );

	map_data.lights.resize( lights_count );
	for( unsigned int l= 0u; l < lights_count; l++ )
	{
		const MapLight& in_light= reinterpret_cast<const MapLight*>( map_file.data() + c_lights_offset )[l];
		MapData::Light& out_light= map_data.lights[l];

		out_light.pos.x= float(in_light.position[0]) * g_map_coords_scale;
		out_light.pos.y= float(in_light.position[1]) * g_map_coords_scale;
		out_light.inner_radius= float(in_light.r0) * g_map_coords_scale;
		out_light.outer_radius= float(in_light.r1) * g_map_coords_scale;
		out_light.power= float(in_light.light_power);

		out_light.max_light_level= 128.0f - float( in_light.bm );

		// In original game and editor if r0 == r1, light source is like with r0=0.
		if( out_light.inner_radius >= out_light.outer_radius )
			out_light.inner_radius= 0.0f;

		// Make just a bit biger
		out_light.outer_radius+= 0.25f;
	}

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
		monster.angle= float( map_monsters[m].angle & 7u) / 8.0f * Constants::two_pi + Constants::pi;

		monster.difficulty_flags= map_monsters[m].difficulty_flags;
	}
}

void MapLoader::LoadMapName( const Vfs::FileContent& resource_file, char* const out_map_name )
{
	out_map_name[0]= '\0';

	const char* s= GetSubstring( reinterpret_cast<const char*>( resource_file.data() ), "#name" );
	if( s == nullptr )
	{
		std::strcpy( out_map_name, "unnamed" );
		return;
	}
	s+= std::strlen( "#name" );

	// Skip '=' and spaces before '='
	while( std::isspace(*s) ) s++;
	s++;

	char* dst= out_map_name;
	while(
		*s != '\0' &&
		! ( *s == '\n' || *s == '\r' ) &&
		dst < out_map_name + MapData::c_max_map_name_size - 1u )
	{
		*dst= *s; dst++; s++;
	}
	*dst= '\0';

}

void MapLoader::LoadSkyTextureName( const Vfs::FileContent& resource_file, MapData& map_data )
{
	map_data.sky_texture_name[0]= '\0';

	const char* s= GetSubstring( reinterpret_cast<const char*>( resource_file.data() ), "#sky" );
	s+= std::strlen( "#sky" );

	while( std::isspace(*s) ) s++;

	// =
	s++;

	while( std::isspace(*s) )s++;

	char* dst= map_data.sky_texture_name;
	while(
		*s != '\0' &&
		!std::isspace( *s ) &&
		dst < map_data.sky_texture_name + sizeof(map_data.sky_texture_name) - 1u )
	{
		*dst= *s; dst++; s++;
	}
	*dst= '\0';
}

void MapLoader::LoadModelsDescription( const Vfs::FileContent& resource_file, MapData& map_data )
{
	const char* start= GetSubstring( reinterpret_cast<const char*>( resource_file.data() ), "#newobjects" );

	while( *start != '\n' ) start++;
	start++;

	const char* const end= GetSubstring( start, "#end" );

	std::istringstream stream( std::string( start, end ) );

	while( !stream.eof() )
	{
		char line[ 512u ];
		stream.getline( line, sizeof(line), '\n' );

		if( stream.eof() )
			break;

		std::istringstream line_stream{ std::string( line ) };

		map_data.models_description.emplace_back();
		MapData::ModelDescription& model_description= map_data.models_description.back();

		line_stream >> model_description.radius; // GoRad

		int shadow= 0;
		line_stream >> shadow; // Shad
		model_description.cast_shadow= shadow != 0; // Stream fails if number is not zero or not one

		line_stream >> model_description.bobj; // BObj
		line_stream >> model_description.bmpz; // BMPz
		line_stream >> model_description.ac; // AC
		line_stream >> model_description.blow_effect; // Blw
		line_stream >> model_description.break_limit; // BLmt
		line_stream >> model_description.ambient_sfx_number; // SFX
		line_stream >> model_description.break_sfx_number; // BSfx

		line_stream >> model_description.file_name; // FileName

		model_description.animation_file_name[0u]= '\0';
		line_stream >> model_description.animation_file_name;

		model_description.radius*= g_map_coords_scale;
	}
}

void MapLoader::LoadWallsTexturesDescription( const Vfs::FileContent& resource_file, MapData& map_data )
{
	for( MapData::WallTextureDescription& tex: map_data.walls_textures )
	{
		tex.file_path[0]= '\0';
		tex.gso[0]= tex.gso[1]= tex.gso[2]= false;
	}

	const char* start= GetSubstring( reinterpret_cast<const char*>( resource_file.data() ), "#GFX" );
	start+= std::strlen( "#GFX" );
	const char* const end= GetSubstring( start, "#end" );

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
		if( line_stream.fail() )
			continue;

		MapData::WallTextureDescription& texture_description= map_data.walls_textures[ texture_number ];

		char texture_name[ MapData::c_max_file_name_size ];
		line_stream >> texture_name;
		if( line_stream.fail() )
			continue;

		if( texture_name[0] != '\0' )
			std::snprintf(
				texture_description.file_path,
				sizeof(MapData::WallTextureDescription::file_path),
				"%s%s", textures_path_, texture_name );
		else
			texture_description.file_path[0]= '\0';

		char gso[16];
		line_stream >> gso;
		if( !line_stream.fail() )
		{
			for( unsigned int j= 0u; j < 3u; j++ )
				if( gso[j] != '.' )
					texture_description.gso[j]= true;
		}
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


void MapLoader::LoadLevelScripts( const Vfs::FileContent& process_file, MapData& map_data )
{
	const char* const start= reinterpret_cast<const char*>( process_file.data() );
	const char* const end= start + process_file.size();

	std::istringstream stream( std::string( start, end ) );

	char line[ 512 ];

	while( !stream.eof() )
	{
		stream.getline( line, sizeof(line), '\n' );
		if( stream.eof() || stream.fail() )
			break;

		std::istringstream line_stream{ std::string( line ) };

		char thing_type[ sizeof(line) ];
		line_stream >> thing_type;

		if( line_stream.fail() || thing_type[0] != '#' )
			continue;
		else if( StringEquals( thing_type, "#mess" ) )
		{
			unsigned int message_number= 0;
			line_stream >> message_number;
			if( !line_stream.fail() && message_number != 0 )
				LoadMessage( message_number, stream, map_data );
		}
		else if( std::strncmp( thing_type, "#proc", std::strlen("#proc" ) ) == 0 )
		{
			const unsigned int c_max_procedure_number= 1000;

			// catch something, like #proc42
			if( std::strlen( thing_type ) > std::strlen( "#proc" ) )
			{
				const char* const num_str= thing_type + std::strlen( "#proc" );
				const unsigned int procedure_number= std::atoi( num_str );
				if( std::isdigit(*num_str) && procedure_number < c_max_procedure_number )
					LoadProcedure( procedure_number, stream, map_data );
			}
			else
			{
				unsigned int procedure_number;
				line_stream >> procedure_number;
				if( !line_stream.fail() && procedure_number < c_max_procedure_number )
					LoadProcedure( procedure_number, stream, map_data );
			}
		}
		else if( StringEquals( thing_type, "#links" ) )
			LoadLinks( stream, map_data );
		else if( StringEquals( thing_type, "#teleports" ) )
			LoadTeleports( stream, map_data );
		else if( StringEquals( thing_type, "#stopani" ) )
		{
			map_data.stopani_commands.emplace_back();
			line_stream >> map_data.stopani_commands.back();
		}

	} // for file
}

void MapLoader::LoadMessage(
	const unsigned int message_number,
	std::istringstream& stream,
	MapData& map_data )
{
	if( message_number >= map_data.messages.size() )
		map_data.messages.resize( message_number + 1u );
	MapData::Message& message= map_data.messages[ message_number ];

	while( !stream.eof() )
	{
		char line[ 512 ];
		stream.getline( line, sizeof(line), '\n' );

		if( stream.eof() )
			break;

		std::istringstream line_stream{ std::string( line ) };

		char thing[64];
		line_stream >> thing;
		if( StringEquals( thing, "#end" ) )
			break;

		else if( StringEquals( thing, "Delay" )  )
			line_stream >> message.delay_s;

		else if( std::strncmp( thing, "Text", std::strlen("Text") ) == 0 )
		{
			message.texts.emplace_back();
			MapData::Message::Text& text= message.texts.back();

			line_stream >> text.x;
			line_stream >> text.y;

			// Read line in ""
			char text_line[512];
			line_stream >> text_line;
			const int len= std::strlen(text_line);
			line_stream.getline( text_line + len, sizeof(text_line) - len, '\n' );

			text.data= std::string( text_line + 1u, text_line + std::strlen( text_line ) - 2u );
		}
	}
}

void MapLoader::LoadProcedure(
	const unsigned int procedure_number,
	std::istringstream& stream,
	MapData& map_data )
{
	if( procedure_number >= map_data.procedures.size() )
		map_data.procedures.resize( procedure_number + 1u );
	MapData::Procedure& procedure= map_data.procedures[ procedure_number ];

	bool has_action= false;

	while( !stream.eof() )
	{
		char line[ 512 ];
		stream.getline( line, sizeof(line), '\n' );

		if( stream.eof() )
			break;

		std::istringstream line_stream{ std::string( line ) };

		char thing[64];
		line_stream >> thing;

		if( line_stream.fail() )
			continue;
		if( StringEquals( thing, "#end" ) )
			break;
		if( thing[0] == ';' )
			continue;

		else if( StringEquals( thing, "StartDelay" ) )
			line_stream >> procedure.start_delay_s;
		else if( StringEquals( thing, "EndDelay" ) )
			line_stream >> procedure.end_delay_s;
		else if( StringEquals( thing, "BackWait" ) )
			line_stream >> procedure.back_wait_s;
		else if( StringEquals( thing, "Speed" ) )
			line_stream >> procedure.speed;
		else if( StringEquals( thing, "checkgo" ) )
			procedure.check_go= true;
		else if( StringEquals( thing, "checkback" ) )
			procedure.check_back= true;
		else if( StringEquals( thing, "Mortal" ) )
			procedure.mortal= true;
		else if( StringEquals( thing, "LightRemap" ) )
			line_stream >> procedure.light_remap;
		else if( StringEquals( thing, "Lock" ) && !has_action ) // Distinguish property "lock" and action command "lock".
			procedure.locked= true;
		else if( StringEquals( thing, "OnMessage" ) )
			line_stream >> procedure.on_message_number;
		else if( StringEquals( thing, "FirstMessage" ) )
			line_stream >> procedure.first_message_number;
		else if( StringEquals( thing, "LockMessage" ) )
			line_stream >> procedure.lock_message_number;
		else if( StringEquals( thing, "SfxId" ) )
			line_stream >> procedure.sfx_id;
		else if( StringEquals( thing, "SfxPosxy" ) )
		{
			int x, y;
			line_stream >> x; line_stream >> y;
			procedure.sfx_pos.emplace_back();
			procedure.sfx_pos.back().x= x;
			procedure.sfx_pos.back().y= y;
		}
		else if( StringEquals( thing, "LinkSwitchAt" ) )
		{
			int x, y;
			line_stream >> x; line_stream >> y;
			procedure.linked_switches.emplace_back();
			procedure.linked_switches.back().x= x;
			procedure.linked_switches.back().y= y;
		}
		else if( StringEquals( thing, "RedKey" ) )
			procedure.red_key_required= true;
		else if( StringEquals( thing, "GreenKey" ) )
			procedure.green_key_required= true;
		else if( StringEquals( thing, "BlueKey" ) )
			procedure.blue_key_required= true;

		else if( StringEquals( thing, "#action" ) )
			has_action= true;
		else if( has_action )
		{
			const auto commnd_id= ActionCommandFormString( thing );
			if( commnd_id == MapData::Procedure::ActionCommandId::Unknown )
				Log::Warning( "Unknown coommand: ", thing );
			else
			{
				procedure.action_commands.emplace_back();
				auto& command= procedure.action_commands.back();

				command.id= commnd_id;

				unsigned int arg= 0u;
				while( !line_stream.fail() )
				{
					line_stream >> command.args[arg];
					arg++;
				}
			}
		} // if has_action

	} // for procedure
}

void MapLoader::LoadLinks( std::istringstream& stream, MapData& map_data )
{
	while( !stream.eof() )
	{
		char line[ 512 ];
		stream.getline( line, sizeof(line), '\n' );

		if( stream.eof() )
			break;

		std::istringstream line_stream{ std::string( line ) };

		char link_type[512];
		line_stream >> link_type;
		if( line_stream.fail() )
			continue;
		if( link_type[0] == ';' )
			continue;
		if( StringEquals( link_type, "#end" ) )
			break;

		map_data.links.emplace_back();
		MapData::Link& link= map_data.links.back();

		unsigned short x, y, proc_id;
		line_stream >> x;
		line_stream >> y;
		line_stream >> proc_id;

		link.x= x;
		link.y= y;
		link.proc_id= proc_id;
		link.type= LinkTypeFromString( link_type );
	}
}

void MapLoader::LoadTeleports( std::istringstream& stream, MapData& map_data )
{
	while( !stream.eof() )
	{
		char line[ 512 ];
		stream.getline( line, sizeof(line), '\n' );

		if( stream.eof() )
			break;

		std::istringstream line_stream{ std::string( line ) };

		char str[512]; // must be "tcenter"
		line_stream >> str;
		if( line_stream.fail() )
			continue;
		if( str[0] == ';' )
			continue;
		if( StringEquals( str, "#end" ) )
			break;

		map_data.teleports.emplace_back();
		MapData::Teleport& teleport= map_data.teleports.back();

		line_stream >> teleport.from[0];
		line_stream >> teleport.from[1];
		line_stream >> teleport.to[0];
		line_stream >> teleport.to[1];

		unsigned int angle;
		line_stream >> angle;
		teleport.angle= -float(angle) / 4.0f * Constants::two_pi - Constants::half_pi;
	}
}

void MapLoader::MarkDynamicWalls( const MapData& map_data, DynamicWallsMask& out_dynamic_walls )
{
	for( bool& wall_is_dynamic : out_dynamic_walls )
		wall_is_dynamic= false;

	for( const MapData::Procedure& procedure : map_data.procedures )
	{
		for( const MapData::Procedure::ActionCommand& command : procedure.action_commands )
		{
			using Command= MapData::Procedure::ActionCommandId;
			if( command.id == Command::Move ||
				command.id == Command::XMove ||
				command.id == Command::YMove ||
				command.id == Command::Rotate ||
				command.id == Command::Up ||
				command.id == Command::Change )
			{
				const unsigned int x= static_cast<unsigned int>(command.args[0]);
				const unsigned int y= static_cast<unsigned int>(command.args[1]);
				if( x < MapData::c_map_size &&
					y < MapData::c_map_size )
				{
					out_dynamic_walls[ x + y * MapData::c_map_size ]= true;
				}
			}
		} // for commands
	} // for procedures
}

void MapLoader::LoadModels( MapData& map_data )
{
	Log::Info( "Loading map models" );

	map_data.models.resize( map_data.models_description.size() );

	Vfs::FileContent file_content;
	Vfs::FileContent animation_file_content;

	for( unsigned int m= 0u; m < map_data.models.size(); m++ )
	{
		const MapData::ModelDescription& model_description= map_data.models_description[m];

		char model_file_path[ MapData::c_max_file_path_size ];
		std::snprintf( model_file_path, sizeof(model_file_path), "%s%s", models_path_, model_description.file_name );
		vfs_->ReadFile( model_file_path, file_content );

		if( model_description.animation_file_name[0u] != '\0' )
		{
			// TODO - know, why some models animations file names have % prefix.
			const char* file_name= model_description.animation_file_name;
			if( file_name[0] == '%' )
				file_name++;

			char animation_file_path[ MapData::c_max_file_path_size ];
			std::snprintf( animation_file_path, sizeof(animation_file_path), "%s%s", animations_path_, file_name );
			vfs_->ReadFile( animation_file_path, animation_file_content );
		}
		else
			animation_file_content.clear();

		LoadModel_o3( file_content, animation_file_content, map_data.models[m] );
	} // for models
}

bool MapLoader::GetMapInfoImpl( const unsigned int map_number, MapInfo& out_map_info )
{
	char level_path[ MapData::c_max_file_path_size ];
	char resource_file_name[ MapData::c_max_file_path_size ];

	std::snprintf( level_path, sizeof(level_path), "LEVEL%02u/", map_number );
	std::snprintf( resource_file_name, sizeof(resource_file_name), "%sRESOURCE.%02u", level_path, map_number );

	const Vfs::FileContent resource_file_content= vfs_->ReadFile( resource_file_name );

	if( resource_file_content.empty() )
		return false;

	char map_name[ MapData::c_max_map_name_size ];
	LoadMapName( resource_file_content, map_name );

	out_map_info.number= map_number;
	out_map_info.name= map_name;

	return true;
}

} // namespace PanzerChasm
