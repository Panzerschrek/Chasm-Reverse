#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>

#include "assert.hpp"
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
		line_stream >> monster_description.attack_radius; // ARad
		line_stream >> monster_description.speed; // SPEED/
		line_stream >> monster_description.rotation_speed; // R
		line_stream >> monster_description.life; // LIFE
		line_stream >> monster_description.kick; // Kick
		line_stream >> monster_description.rock; // Rock
		line_stream >> monster_description.sep_limit; // SepLimit

		monster_description.w_radius/= 256.0f;
		monster_description.attack_radius/= 256.0f;

		i++;
	}
}

static void LoadSpriteEffectsDescription(
	const Vfs::FileContent& inf_file,
	GameResources& game_resources )
{
	const char* const effects_start=
		std::strstr( reinterpret_cast<const char*>(inf_file.data()), "[BLOWS]" );
	const char* const effects_end= reinterpret_cast<const char*>(inf_file.data()) + inf_file.size() - 1u;

	std::istringstream stream( std::string( effects_start, effects_end ) );

	char line[ 512 ];
	stream.getline( line, sizeof(line), '\n' );

	unsigned int effects_count= 0u;
	stream >> effects_count;
	stream.getline( line, sizeof(line), '\n' );

	game_resources.sprites_effects_description.resize( effects_count );

	for( unsigned int i= 0u; i < effects_count; )
	{
		stream.getline( line, sizeof(line), '\n' );
		if( line[0] == ';' )
			continue;

		std::istringstream line_stream{ std::string( line ) };

		GameResources::SpriteEffectDescription& effect_description= game_resources.sprites_effects_description[i];

		line_stream >> effect_description.glass;
		line_stream >> effect_description.half_size;
		line_stream >> effect_description.smooking;
		line_stream >> effect_description.looped;
		line_stream >> effect_description.gravity;
		line_stream >> effect_description.jump;
		line_stream >> effect_description.light_on;
		line_stream >> effect_description.sprite_file_name;

		i++;
	}
}

static void LoadBMPObjectsDescription(
	const Vfs::FileContent& inf_file,
	GameResources& game_resources )
{
	const char* const bmp_start=
		std::strstr( reinterpret_cast<const char*>(inf_file.data()), "[BMP_OBJECTS]" );
	const char* const bmp_end= reinterpret_cast<const char*>(inf_file.data()) + inf_file.size() - 1u;

	std::istringstream stream( std::string( bmp_start, bmp_end ) );

	char line[ 512 ];
	stream.getline( line, sizeof(line), '\n' );

	unsigned int bmp_count= 0u;
	stream >> bmp_count;
	stream.getline( line, sizeof(line), '\n' );

	game_resources.bmp_objects_description.resize( bmp_count );

	for( unsigned int i= 0u; i < bmp_count; )
	{
		stream.getline( line, sizeof(line), '\n' );
		if( line[0] == ';' )
			continue;

		std::istringstream line_stream{ std::string( line ) };

		GameResources::BMPObjectDescription& description= game_resources.bmp_objects_description[i];
		line_stream >> description.light;
		line_stream >> description.glass;
		line_stream >> description.half_size;

		int zero;
		line_stream >> zero; line_stream >> zero; line_stream >> zero;

		line_stream >> description.sprite_file_name;

		i++;
	}
}

static void LoadWeaponsDescription(
	const Vfs::FileContent& inf_file,
	GameResources& game_resources )
{
	const char* const weapons_start=
		std::strstr( reinterpret_cast<const char*>(inf_file.data()), "[WEAPONS]" );
	const char* const weapons_end= reinterpret_cast<const char*>(inf_file.data()) + inf_file.size() - 1u;

	std::istringstream stream( std::string( weapons_start, weapons_end ) );

	char line[ 512 ];
	stream.getline( line, sizeof(line), '\n' );

	unsigned int weapon_count= 0u;
	stream >> weapon_count;
	stream.getline( line, sizeof(line), '\n' );

	game_resources.weapons_description.resize( weapon_count );

	for( unsigned int i= 0u; i < weapon_count; i++ )
	{
		GameResources::WeaponDescription& weapon_description= game_resources.weapons_description[i];

		for( unsigned int j= 0u; j < 3u; j++ )
		{
			stream.getline( line, sizeof(line), '\n' );

			const char* param_start= line;
			while( std::isspace( *param_start ) ) param_start++;

			const char* param_end= param_start;
			while( std::isalpha( *param_end ) ) param_end++;

			const char* value_start= param_end;
			while( std::isspace( *value_start ) ) value_start++;
			value_start++;
			while( std::isspace( *value_start ) ) value_start++;

			const char* value_end= value_start;
			while( !std::isspace( *value_end ) ) value_end++;

			const unsigned int param_length= param_end - param_start;
			const unsigned int value_length= value_end - value_start;

			if( std::strncmp( param_start, "MODEL", param_length ) == 0 )
				std::strncpy( weapon_description.model_file_name, value_start, value_length );
			else if( std::strncmp( param_start, "STAT", param_length ) == 0 )
				std::strncpy( weapon_description.animation_file_name, value_start, value_length );
			else if( std::strncmp( param_start, "SHOOT", param_length ) == 0 )
				std::strncpy( weapon_description.reloading_animation_file_name, value_start, value_length );
		}

		stream.getline( line, sizeof(line), '\n' );
		std::istringstream line_stream{ std::string( line ) };

		line_stream >> weapon_description.r_type;
		line_stream >> weapon_description.reloading_time;
		line_stream >> weapon_description.y_sh;
		line_stream >> weapon_description.r_z0;
		line_stream >> weapon_description.d_am;
		line_stream >> weapon_description.limit;
		line_stream >> weapon_description.start;
		line_stream >> weapon_description.r_count;
	}
}

static void LoadRocketsDescription(
	const Vfs::FileContent& inf_file,
	GameResources& game_resources )
{
	const char* const rockets_start=
		std::strstr( reinterpret_cast<const char*>(inf_file.data()), "[ROCKETS]" );
	const char* const rockets_end= reinterpret_cast<const char*>(inf_file.data()) + inf_file.size() - 1u;

	std::istringstream stream( std::string( rockets_start, rockets_end ) );

	char line[ 512 ];
	stream.getline( line, sizeof(line), '\n' );

	unsigned int rockets_count= 0u;
	stream >> rockets_count;
	stream.getline( line, sizeof(line), '\n' );
	stream.getline( line, sizeof(line), '\n' );

	game_resources.rockets_description.resize( rockets_count );

	for( unsigned int i= 0u; i < rockets_count; i++ )
	{
		GameResources::RocketDescription& rocket_description= game_resources.rockets_description[i];

		rocket_description.model_file_name[0]= '\0';
		rocket_description.animation_file_name[0]= '\0';

		for( unsigned int j= 0u; j < 2u; j++ )
		{
			stream.getline( line, sizeof(line), '\n' );
			std::istringstream line_stream{ std::string( line ) };

			char param_name[64], param_value[64], eq[16];
			param_name[0]= param_value[0]= '\0';

			line_stream >> param_name;
			line_stream >> eq;
			line_stream >> param_value;

			if( param_value[0] == ';' )
				continue;

			if( std::strcmp( param_name, "3d_MODEL" ) == 0u )
				std::strcpy( rocket_description.model_file_name, param_value );
			else if( std::strcmp( param_name, "ANIMATION" ) == 0u )
				std::strcpy( rocket_description.animation_file_name, param_value );
		}

		stream.getline( line, sizeof(line), '\n' );
		std::istringstream line_stream{ std::string( line ) };

		line_stream >> rocket_description.blow_effect; // BW
		line_stream >> rocket_description.gravity_force; // GF
		line_stream >> rocket_description.explosion_radius; // Ard
		line_stream >> rocket_description.CRd; // CRd
		line_stream >> rocket_description.power; // Pwr

		line_stream >> rocket_description.reflect; // Rfl
		line_stream >> rocket_description.fullbright; // FBright
		line_stream >> rocket_description.Light; // Light
		line_stream >> rocket_description.Auto; // Auto
		line_stream >> rocket_description.Auto2; // Auto2
		line_stream >> rocket_description.fast; // Fast
		line_stream >> rocket_description.smoke_trail_effect_id; // Smoke

		stream.getline( line, sizeof(line), '\n' );

		rocket_description.explosion_radius /= 256.0f;
	}
}

static void LoadGibsDescription(
	const Vfs::FileContent& inf_file,
	GameResources& game_resources )
{
	const char* const gibs_start=
		std::strstr( reinterpret_cast<const char*>(inf_file.data()), "[GIBS]" );
	const char* const gibs_end= reinterpret_cast<const char*>(inf_file.data()) + inf_file.size() - 1u;

	std::istringstream stream( std::string( gibs_start, gibs_end ) );

	char line[ 512 ];
	stream.getline( line, sizeof(line), '\n' );

	game_resources.gibs_description.reserve( 16u );

	unsigned int gib_number= 0u;
	while( !stream.fail() )
	{
		stream.getline( line, sizeof(line), '\n' );
		if( stream.eof() )
			break;
		if( std::strncmp( line, "#end", std::strlen( "#end" ) ) == 0 )
			break;

		const char* s= line;

		//const unsigned int gib_number= std::atoi(s) - GameResources::c_first_gib_number;

		while( std::isdigit( *s ) ) s++;
		while( std::isspace( *s ) ) s++;
		s++; // :
		while( *s != '=' ) s++;
		s++; // =
		while( std::isspace( *s ) && *s != '\0' ) s++;

		if( *s == '\0' )
			continue;

		if( gib_number >= game_resources.gibs_description.size() )
			game_resources.gibs_description.resize( gib_number + 1u );

		GameResources::GibDescription& gib= game_resources.gibs_description[ gib_number ];
		gib.model_file_name[0]= '\0';
		gib.sound_number= 0u;

		char* file_name_dst= gib.model_file_name;
		while( !std::isspace( *s ) && *s != '\0' )
		{
			*file_name_dst= *s;
			file_name_dst++;
			s++;
		}
		*file_name_dst= '\0';

		while( std::isspace( *s ) && *s != '\0' ) s++;

		if( *s == 's' || *s == 's' )
		{
			s+=2u;// s:
			gib.sound_number= std::atoi( s );
		}

		gib_number++;
	}
}

static void LoadSoundsDescriptionFromFileData(
	const char* start, const char* end,
	unsigned int first_sound,
	GameResources::SoundDescription* out_sounds )
{
	std::istringstream stream( std::string( start, end ) );

	char line[ 512 ];
	stream.getline( line, sizeof(line), '\n' ); // skip [SOUNDS]

	while( !stream.eof() )
	{
		stream.getline( line, sizeof(line), '\n' );
		if( stream.eof() )
			break;

		const char* s= line;

		const unsigned int sound_number= std::atoi(s) - first_sound;

		while( std::isdigit( *s ) ) s++;
		while( std::isspace( *s ) ) s++;
		s++; // :
		while( *s != '=' ) s++;
		s++; // =
		while( std::isspace( *s ) && *s != '\0' ) s++;

		if( *s == '\0' )
			continue;

		GameResources::SoundDescription& sound= out_sounds[ sound_number ];
		char* file_name_dst= sound.file_name;
		while( !std::isspace( *s ) && *s != '\0' )
		{
			*file_name_dst= *s;
			file_name_dst++;
			s++;
		}
		*file_name_dst= '\0';

		std::replace(sound.file_name, file_name_dst, '\\', '/');

		while( std::isspace( *s ) && *s != '\0' ) s++;
		if( *s == 'v' || *s == 'V' )
		{
			s+=2u;// v:
			sound.volume= std::atoi( s );
		}
		else
			sound.volume= GameResources::SoundDescription::c_max_volume;
	}
}

static void LoadSoundsDescription(
	const Vfs::FileContent& inf_file,
	GameResources& game_resources )
{
	for( GameResources::SoundDescription& sound : game_resources.sounds )
	{
		sound.file_name[0]= '\0';
		sound.volume= 0u;
	}

	const char* const start=
		std::strstr( reinterpret_cast<const char*>(inf_file.data()), "[SOUNDS]" );
	const char* const end= std::strstr( start, "[SOUNDS_END]" );

	LoadSoundsDescriptionFromFileData( start, end, 0u, game_resources.sounds );
}

static void LoadItemsModels(
	const Vfs& vfs,
	GameResources& game_resources )
{
	game_resources.items_models.resize( game_resources.items_description.size() );

	Vfs::FileContent file_content;
	Vfs::FileContent animation_file_content;

	for( unsigned int i= 0u; i < game_resources.items_models.size(); i++ )
	{
		const GameResources::ItemDescription& item_description= game_resources.items_description[i];
		const std::filesystem::path model_file_name(item_description.model_file_name);
		const std::filesystem::path animation_file_name(item_description.animation_file_name);

		std::filesystem::path model_file_path( "MODELS/" / model_file_name);
		std::filesystem::path animation_file_path( "MODELS/" / animation_file_name);

		vfs.ReadFile( model_file_path, file_content );

		if( !animation_file_name.empty() )
			vfs.ReadFile( animation_file_path, animation_file_content );
		else
			animation_file_content.clear();

		LoadModel_o3( file_content, animation_file_content, game_resources.items_models[i] );
	}
}

static void LoadMonstersModels(
	const Vfs& vfs,
	GameResources& game_resources )
{
	game_resources.monsters_models.resize( game_resources.monsters_description.size() );

	Vfs::FileContent file_content;

	for( unsigned int i= 0u; i < game_resources.monsters_models.size(); i++ )
	{
		const GameResources::MonsterDescription& monster_description= game_resources.monsters_description[i];
		const std::filesystem::path model_file_name( monster_description.model_file_name );

		std::filesystem::path model_file_path( "CARACTER/" / model_file_name );

		vfs.ReadFile( model_file_path, file_content );
		LoadModel_car( file_content, game_resources.monsters_models[i] );
	}
}

static void LoadEffectsSprites(
	const Vfs& vfs,
	GameResources& game_resources )
{
	game_resources.effects_sprites.resize( game_resources.sprites_effects_description.size() );

	Vfs::FileContent file_content;

	for( unsigned int i= 0u; i < game_resources.effects_sprites.size(); i++ )
	{
		const std::filesystem::path sprite_file_name( game_resources.sprites_effects_description[i].sprite_file_name );

		vfs.ReadFile( sprite_file_name, file_content );
		LoadObjSprite( file_content, game_resources.effects_sprites[i] );
	}
}

static void LoadBMPObjectsSprites(
	const Vfs& vfs,
	GameResources& game_resources )
{
	game_resources.bmp_objects_sprites.resize( game_resources.bmp_objects_description.size() );
	Vfs::FileContent file_content;

	for( unsigned int i= 0u; i < game_resources.bmp_objects_sprites.size(); i++ )
	{
		const std::filesystem::path sprite_file_name( game_resources.bmp_objects_description[i].sprite_file_name );
		vfs.ReadFile( sprite_file_name, file_content );
		LoadObjSprite( file_content, game_resources.bmp_objects_sprites[i] );
	}
}


static void LoadWeaponsModels(
	const Vfs& vfs,
	GameResources& game_resources )
{
	game_resources.weapons_models.resize( game_resources.weapons_description.size() );

	Vfs::FileContent file_content;
	Vfs::FileContent animation_file_content[2u];

	for( unsigned int i= 0u; i < game_resources.weapons_models.size(); i++ )
	{
		const GameResources::WeaponDescription& weapon_description= game_resources.weapons_description[i];
		const std::filesystem::path model_file_name( weapon_description.model_file_name );
		const std::filesystem::path animation_file_name( weapon_description.animation_file_name );
		const std::filesystem::path reloading_animation_file_name( weapon_description.reloading_animation_file_name );

		std::filesystem::path model_file_path( "MODELS/" / model_file_name );
		std::filesystem::path animation_file_path( "ANI/WEAPON/" / animation_file_name );
		std::filesystem::path reloading_animation_file_path( "ANI/WEAPON/" / reloading_animation_file_name );

		vfs.ReadFile( model_file_path, file_content );
		vfs.ReadFile( animation_file_path, animation_file_content[0] );
		vfs.ReadFile( reloading_animation_file_path, animation_file_content[1] );

		LoadModel_o3( file_content, animation_file_content, 2u, game_resources.weapons_models[i] );
	}
}

static void LoadRocketsModels(
	const Vfs& vfs,
	GameResources& game_resources )
{
	game_resources.rockets_models.resize( game_resources.rockets_description.size() );

	Vfs::FileContent file_content;
	Vfs::FileContent animation_file_content;

	for( unsigned int i= 0u; i < game_resources.rockets_models.size(); i++ )
	{
		const GameResources::RocketDescription& rocket_description= game_resources.rockets_description[i];
		const std::filesystem::path model_file_name( rocket_description.model_file_name );
		const std::filesystem::path animation_file_name( rocket_description.animation_file_name );

		if( model_file_name.empty() )
			continue;

		std::filesystem::path model_file_path( "MODELS/" / model_file_name );
		std::filesystem::path animation_file_path( "ANI/" / animation_file_name );


		vfs.ReadFile( model_file_path, file_content );
		if( !animation_file_name.empty() )
			vfs.ReadFile( animation_file_path, animation_file_content );
		else
			animation_file_content.clear();

		LoadModel_o3( file_content, animation_file_content, game_resources.rockets_models[i] );
	}
}

static void LoadGibsModels(
	const Vfs& vfs,
	GameResources& game_resources )
{
	game_resources.gibs_models.resize( game_resources.gibs_description.size() );

	Vfs::FileContent model_file_content;

	for( unsigned int i= 0u; i < game_resources.gibs_models.size(); i++ )
	{
		const GameResources::GibDescription& gib_description= game_resources.gibs_description[i];
		const std::filesystem::path model_file_name= gib_description.model_file_name;
		if( gib_description.model_file_name[0] == '\0' )
			continue;

		std::filesystem::path model_file_path( "MODELS/" / model_file_name );

		vfs.ReadFile( model_file_path, model_file_content );
		LoadModel_o3( model_file_content, Vfs::FileContent(), game_resources.gibs_models[i] );
	}
}

GameResourcesConstPtr LoadGameResources( const VfsPtr& vfs )
{
	PC_ASSERT( vfs != nullptr );

	const GameResourcesPtr result= std::make_shared<GameResources>();

	result->vfs= vfs;

	LoadPalette( *vfs, result->palette );

	const Vfs::FileContent inf_file= vfs->ReadFile( "CHASM.INF" );

	if( inf_file.empty() )
		Log::FatalError( "Can not read CHASM.INF" );

	LoadItemsDescription( inf_file, *result );
	LoadMonstersDescription( inf_file, *result );
	LoadSpriteEffectsDescription( inf_file, *result );
	LoadBMPObjectsDescription( inf_file, *result );
	LoadWeaponsDescription( inf_file, *result );
	LoadRocketsDescription( inf_file, *result );
	LoadGibsDescription( inf_file, *result );
	LoadSoundsDescription( inf_file, *result );

	LoadItemsModels( *vfs, *result );
	LoadMonstersModels( *vfs, *result );
	LoadEffectsSprites( *vfs, *result );
	LoadBMPObjectsSprites( *vfs, *result );
	LoadWeaponsModels( *vfs, *result );
	LoadRocketsModels( *vfs, *result );
	LoadGibsModels( *vfs, *result );

	return result;
}

void LoadSoundsDescriptionFromMapResourcesFile(
	const Vfs::FileContent& resoure_file,
	GameResources::SoundDescription* const out_sounds,
	const unsigned int max_sound_count )
{
	for( unsigned int s= 0u; s < max_sound_count; s++ )
	{
		out_sounds[s].file_name[0]= '\0';
		out_sounds[s].volume= 0u;
	}

	// Use std::search instead of std::strstr, because input string is not null-terminated.
	const char* const file_begin= reinterpret_cast<const char*>(resoure_file.data());
	const char* const file_end= file_begin + resoure_file.size();
	const char* const str= "#newsounds";
	const char* const start= std::search( file_begin, file_end, str, str + std::strlen(str) );
	if( start == file_end )
		return;

	const char* const end= std::strstr( start, "#end" );

	LoadSoundsDescriptionFromFileData( start, end, GameResources::c_max_global_sounds, out_sounds );
}

void LoadAmbientSoundsDescriptionFromMapResourcesFile(
	const Vfs::FileContent& resoure_file,
	GameResources::SoundDescription* out_sounds,
	unsigned int max_sound_count )
{
	for( unsigned int s= 0u; s < max_sound_count; s++ )
	{
		out_sounds[s].file_name[0]= '\0';
		out_sounds[s].volume= 0u;
	}

	const char* const start= std::strstr( reinterpret_cast<const char*>(resoure_file.data()), "#ambients" );
	if( start == nullptr )
		return;

	const char* const end= std::strstr( start, "#end" );

	LoadSoundsDescriptionFromFileData( start, end, 0u, out_sounds );
}

} // namespace PanzerChasm
