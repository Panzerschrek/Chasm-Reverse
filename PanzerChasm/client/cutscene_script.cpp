#include <cstring>

#include "../math_utils.hpp"

#include "cutscene_script.hpp"

namespace PanzerChasm
{

static void LoadSetupData( const Vfs::FileContent& file_content, CutsceneScript& script )
{
	const char* const setup= std::strstr( reinterpret_cast<const char*>(file_content.data()), "#setup" );
	const char* const setup_end= std::strstr( setup + std::strlen("#setup"), "#" );

	std::istringstream stream( std::string( setup + std::strlen("#setup"), setup_end ) );

	while( !stream.eof() )
	{
		char line[ 512u ];
		stream.getline( line, sizeof(line), '\n' );
		if( stream.eof() )
			break;

		if( std::strncmp( line, "camera:", std::strlen( "camera:" ) ) == 0 )
		{
			std::istringstream line_stream( std::string( line + std::strlen( "camera:" ) ) );
			line_stream >> script.camera_params.pos.x;
			line_stream >> script.camera_params.pos.y;
			line_stream >> script.camera_params.pos.z;
			line_stream >> script.camera_params.angle;

			script.camera_params.pos/= 64.0f;
			script.camera_params.angle*= Constants::to_rad;
		}
		else if( std::strncmp( line, "ambient:", std::strlen( "ambient:" ) ) == 0 )
			script.ambient_sound_number= std::atoi( line + std::strlen( "ambient:" ) );
		else if( std::strncmp( line, "room:", std::strlen( "room:" ) ) == 0 )
			script.room_number= std::atoi( line + std::strlen( "room:" ) );
		else if( std::strncmp( line, "character", std::strlen( "character" ) ) == 0 )
		{
			const unsigned int character_number= std::atoi( line + std::strlen( "character" ) );
			if( character_number >= script.characters.size() )
				script.characters.resize( character_number + 1u );

			CutsceneScript::Character& character= script.characters[character_number];
			std::memset( character.model_file_name, 0, sizeof( character.model_file_name ) );
			std::memset( character.idle_animation_file_name, 0, sizeof( character.idle_animation_file_name ) );
			std::memset( character.animations_file_name, 0, sizeof( character.animations_file_name ) );

			while( !stream.eof() )
			{
				char character_line[ 512u ];
				stream.getline( character_line, sizeof(character_line), '\n' );
				if( stream.eof() )
					break;

				std::istringstream line_stream{ std::string( character_line ) };
				char line_start[ 64 ];
				line_stream >> line_start;

				if( std::strcmp( line_start, "model" ) == 0 )
				{
					line_stream >> character.model_file_name;
				}
				else if( std::strcmp( line_start, "position" ) == 0 )
				{
					line_stream >> character.pos.x;
					line_stream >> character.pos.y;
					line_stream >> character.pos.z;
					line_stream >> character.angle;

					character.pos.x/= 128.0f;
					character.pos.y/= 128.0f;
					character.pos.z/= -8192.0f; // TODO - calibrate z
					character.angle*= Constants::to_rad;
				}
				else if( std::strcmp( line_start, "idle" ) == 0 )
				{
					int num;
					line_stream >> num;
					line_stream >> character.idle_animation_file_name;
				}
				else if( std::strcmp( line_start, "ani" ) == 0 )
				{
					unsigned int animation_number;
					line_stream >> animation_number;

					if( animation_number < CutsceneScript::c_max_character_animations )
						line_stream >> character.animations_file_name[ animation_number ];
				}
				else if( std::strncmp( line_start, "end", std::strlen( "end" ) ) == 0 )
					break;
			}
		}
	} // while stream not ended
}

static void LoadAction( const Vfs::FileContent& file_content, CutsceneScript& script )
{
	const char* const action= std::strstr( reinterpret_cast<const char*>(file_content.data()), "#action" );
	const char* const action_end= std::strstr( action + std::strlen("#action"), "#end" );

	std::istringstream stream( std::string( action + std::strlen("#action"), action_end ) );

	while( !stream.eof() )
	{
		char line[ 512u ];
		stream.getline( line, sizeof(line), '\n' );
		if( stream.eof() )
			break;

		if( line[0] == ';' ) // comment
			continue;

		std::istringstream line_stream{ std::string( line ) };
		char line_start[ 64 ];
		line_stream >> line_start;
		if( line_stream.fail() )
			continue;

		CutsceneScript::ActionCommand::Type command_type= CutsceneScript::ActionCommand::Type::None;

		if( std::strcmp( line_start, "delay" ) == 0 )
			command_type= CutsceneScript::ActionCommand::Type::Delay;
		else if( std::strcmp( line_start, "say" ) == 0 )
			command_type= CutsceneScript::ActionCommand::Type::Say;
		else if( std::strcmp( line_start, "voice" ) == 0 )
			command_type= CutsceneScript::ActionCommand::Type::Voice;
		else if( std::strcmp( line_start, "setani" ) == 0 )
			command_type= CutsceneScript::ActionCommand::Type::Setani;
		else if( std::strcmp( line_start, "wait_key" ) == 0 )
			command_type= CutsceneScript::ActionCommand::Type::WaitKey;
		else if( std::strcmp( line_start, "movecamto" ) == 0 )
			command_type= CutsceneScript::ActionCommand::Type::MoveCamTo;

		if( command_type != CutsceneScript::ActionCommand::Type::None )
		{
			script.commands.emplace_back();
			CutsceneScript::ActionCommand& command= script.commands.back();
			command.type= command_type;

			// Load params
			for( unsigned int i= 0u; i < CutsceneScript::ActionCommand::c_max_params; i++ )
			{
				std::string& out_param= command.params[i];

				char text_line[512];
				line_stream >> text_line;
				if (text_line[0] == '"' )
				{
					if( std::strcmp( text_line, "\"\"" ) != 0 )
					{
						// Read line in ""
						const int len= std::strlen(text_line);
						line_stream.getline( text_line + len, sizeof(text_line) - len, '"' );

						out_param= text_line + 1u;
					}
				}
				else
					out_param= text_line;

				if( line_stream.fail() )
					break;
			}
		}
	}
}

CutsceneScriptPtr LoadCutsceneScript( const Vfs& vfs, unsigned int map_number )
{
	if( map_number >= 100 )
		return nullptr;

	char level_path[ MapData::c_max_file_path_size ];
	char script_file_name[ MapData::c_max_file_path_size ];

	std::snprintf( level_path, sizeof(level_path), "LEVEL%02u/", map_number );
	std::snprintf( script_file_name, sizeof(script_file_name), "%sSCRIPT.%02u", level_path, map_number );

	const Vfs::FileContent file_content= vfs.ReadFile( script_file_name );
	if( file_content.empty() )
		return nullptr;

	CutsceneScriptPtr result= std::make_shared<CutsceneScript>();

	LoadSetupData( file_content, *result );
	LoadAction( file_content, *result );

	return result;
}

} // namespace PanzerChasm
