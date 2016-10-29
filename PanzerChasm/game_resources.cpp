#include <cstring>
#include <sstream>

#include "log.hpp"

#include "game_resources.hpp"

namespace PanzerChasm
{

static void LoadItemsDescription(
	const Vfs::FileContent& inf_file,
	GameResources& game_resources )
{
	const char* const items_start=
		std::strstr( reinterpret_cast<const char*>(inf_file.data()), "[3D_OBJECTS]" );
	const char* const items_end= reinterpret_cast<const char*>(inf_file.data()) + inf_file.size() - 1u;

	std::istringstream stream( std::string( items_start, items_end ) );

	char line[ 512 ];
	stream.getline( line, sizeof(line), '\n' );

	unsigned int items_count= 0u;
	stream >> items_count;
	stream.getline( line, sizeof(line), '\n' );

	game_resources.items_description.resize( items_count );
	for( unsigned int i= 0u; i < items_count; )
	{
		stream.getline( line, sizeof(line), '\n' );
		if( line[0] == ';' )
			continue;

		std::istringstream line_stream{ std::string( line ) };

		GameResources::ItemDescription& item_description= game_resources.items_description[i];

		line_stream >> item_description.radius; // GoRad
		line_stream >> item_description.cast_shadow; // Shad
		line_stream >> item_description.bmp_obj; // BObj
		line_stream >> item_description.bmp_z; // BMPz
		line_stream >> item_description.a_code; // AC
		line_stream >> item_description.blow_up; // Blw
		line_stream >> item_description.b_limit; // BLmt
		line_stream >> item_description.b_sfx; // BSfx
		line_stream >> item_description.sfx; // SFX

		line_stream >> item_description.model_file_name; // FileName

		item_description.animation_file_name[0]= '\0';
		line_stream >> item_description.animation_file_name; // Animation

		i++;
	}
}

static void LoadMonstersDescription(
	const Vfs::FileContent& inf_file,
	GameResources& game_resources )
{
	const char* const monsters_start=
		std::strstr( reinterpret_cast<const char*>(inf_file.data()), "[MONSTERS]" );
	const char* const monsters_end= reinterpret_cast<const char*>(inf_file.data()) + inf_file.size() - 1u;

	std::istringstream stream( std::string( monsters_start, monsters_end ) );

	char line[ 512 ];
	stream.getline( line, sizeof(line), '\n' );

	unsigned int monsters_count= 0u;
	stream >> monsters_count;
	stream.getline( line, sizeof(line), '\n' );

	game_resources.monsters_description.resize( monsters_count );

	for( unsigned int i= 0u; i < monsters_count; )
	{
		stream.getline( line, sizeof(line), '\n' );
		if( line[0] == ';' )
			continue;

		std::istringstream line_stream{ std::string( line ) };

		GameResources::MonsterDescription& monster_description= game_resources.monsters_description[i];

		line_stream >> monster_description.model_file_name;
		line_stream >> monster_description.w_radius; // WRad
		line_stream >> monster_description.a_radius; // ARad
		line_stream >> monster_description.speed; // SPEED/
		line_stream >> monster_description.r; // R
		line_stream >> monster_description.life; // LIFE
		line_stream >> monster_description.kick; // Kick
		line_stream >> monster_description.rock; // Rock
		line_stream >> monster_description.sep_limit; // SepLimit

		i++;
	}
}

GameResourcesConstPtr LoadGameResources( const VfsPtr& vfs )
{
	const GameResourcesPtr result= std::make_shared<GameResources>();

	result->vfs= vfs;

	LoadPalette( *vfs, result->palette );

	const Vfs::FileContent inf_file= vfs->ReadFile( "CHASM.INF" );

	if( inf_file.empty() )
		Log::FatalError( "Can not read CHASM.INF" );

	LoadItemsDescription( inf_file, *result );
	LoadMonstersDescription( inf_file, *result );

	return result;
}

} // namespace PanzerChasm
