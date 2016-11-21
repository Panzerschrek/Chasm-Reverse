#include <cstring>

#include "assert.hpp"
#include "log.hpp"
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
	unsigned char difficulty_flags;
};

SIZE_ASSERT( MapMonster, 8 );

#pragma pack(pop)

namespace
{

// Case-unsensitive strings equality-comparision
static bool StringEquals( const char* const s0, const char* const s1 )
{
	unsigned int i= 0;
	while( s0[i] != '\0' && s1[i] != '\0' )
	{
		if( std::tolower( s0[i] ) != std::tolower( s1[i] ) )
			return false;
		i++;
	}

	return std::tolower( s0[i] ) == std::tolower( s1[i] );
}

// Case-unsensitive substring search
static const char* GetSubstring( const char* const search_where, const char* const search_what )
{
	const char* str= search_where;
	while( *str != '\0' )
	{
		unsigned int i= 0u;

		while( str[i] != '\0' && search_what[i] != '\0' &&
				std::tolower( str[i] ) == std::tolower( search_what [i] ) )
			i++;

		if( search_what [i] == '\0' )
			return str;

		str++;
	}

	return nullptr;
}

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

	if( map_number == last_loaded_map_number_ && last_loaded_map_ != nullptr )
		return last_loaded_map_;

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
	LoadLightmap( map_file_content,*result );
	LoadWalls( map_file_content, *result, dynamic_walls_mask );
	LoadFloorsAndCeilings( map_file_content,*result );
	LoadMonsters( map_file_content, *result );

	// Scan resource file
	LoadModelsDescription( resource_file_content, *result );
	LoadWallsTexturesDescription( resource_file_content, *result );

	// Scan floors file
	LoadFloorsTexturesData( floors_file_content, *result );

	LoadModels( *result );

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
			255u - in_data[ x + y * MapData::c_lightmap_size ] * 6u;
	}
}

void MapLoader::LoadWalls( const Vfs::FileContent& map_file, MapData& map_data, const DynamicWallsMask& dynamic_walls_mask )
{
	const unsigned int c_walls_offset= 0x18001u;

	for( unsigned int y= 0u; y < MapData::c_map_size; y++ )
	for( unsigned int x= 1u; x < MapData::c_map_size; x++ )
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
				model.is_dynamic= is_dynamic;

				index_element.type= MapData::IndexElement::StaticModel;
				index_element.index= map_data.static_models.size() - 1u;
			}
			else if( map_wall.texture_id >= c_first_item )
			{
				map_data.items.emplace_back();
				MapData::Item& model= map_data.items.back();
				model.pos.x= float(map_wall.vert_coord[0][0]) * g_map_coords_scale;
				model.pos.y= float(map_wall.vert_coord[0][1]) * g_map_coords_scale;
				model.angle= float(map_wall.unknown & 7u) / 8.0f * Constants::two_pi + Constants::pi;
				model.item_id=  map_wall.texture_id - c_first_item;

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

		monster.difficulty_flags= map_monsters[m].difficulty_flags;
	}
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
		line_stream >> model_description.cast_shadow; // Shad
		line_stream >> model_description.bobj; // BObj
		line_stream >> model_description.bmpz; // BMPz
		line_stream >> model_description.ac; // AC
		line_stream >> model_description.blw; // Blw
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
	stream.getline( line, sizeof(line), '\n' );

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
			// catch something, like #proc42
			if( std::strlen( thing_type ) > std::strlen( "#proc" ) )
			{
				const unsigned int procedure_number= std::atoi( thing_type + std::strlen( "#proc" ) );
				if( procedure_number != 0u && procedure_number < 1000 )
					LoadProcedure( procedure_number, stream, map_data );
			}
			else
			{
				unsigned int procedure_number= 0;
				line_stream >> procedure_number;
				if( !line_stream.fail() && procedure_number != 0 )
					LoadProcedure( procedure_number, stream, map_data );
			}
		}
		else if( StringEquals( thing_type, "#links" ) )
			LoadLinks( stream, map_data );
		else if( StringEquals( thing_type, "#stopani" ) )
		{ /* TODO */ }

	} // for file

	return;
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

	procedure.sfx_pos[0]= procedure.sfx_pos[1]= 255;

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
		else if( StringEquals( thing, "BackWait" ) )
			line_stream >> procedure.back_wait_s;
		else if( StringEquals( thing, "Speed" ) )
			line_stream >> procedure.speed;
		else if( StringEquals( thing, "LifeCheckon" ) )
			line_stream >> procedure.life_check;
		else if( StringEquals( thing, "Mortal" ) )
			line_stream >> procedure.mortal;
		else if( StringEquals( thing, "LightRemap" ) )
			line_stream >> procedure.light_remap;
		else if( StringEquals( thing, "Lock" ) )
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
			procedure.sfx_pos[0]= x;
			procedure.sfx_pos[1]= y;
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
				command.id == Command::Up )
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
	map_data.models.resize( map_data.models_description.size() );

	Vfs::FileContent file_content;
	Vfs::FileContent animation_file_content;

	for( unsigned int m= 0u; m < map_data.models.size(); m++ )
	{
		const MapData::ModelDescription& model_description= map_data.models_description[m];

		char model_file_name[ MapData::c_max_file_path_size ];
		char animation_file_name[ MapData::c_max_file_path_size ];

		std::snprintf( model_file_name, sizeof(model_file_name), "%s%s", models_path_, model_description.file_name );
		std::snprintf( animation_file_name, sizeof(animation_file_name), "%s%s", animations_path_, model_description.animation_file_name );

		vfs_->ReadFile( model_file_name, file_content );

		if( model_description.animation_file_name[0u] != '\0' )
			vfs_->ReadFile( animation_file_name, animation_file_content );
		else
			animation_file_content.clear();

		LoadModel_o3( file_content, animation_file_content, map_data.models[m] );
	} // for models
}

} // namespace PanzerChasm
