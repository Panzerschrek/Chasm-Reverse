#include <cctype>
#include <cstdio>
#include <cstring>

#include "shaders_loading.hpp"

static ShaderLoadingLogCallback g_shader_loading_log_callback= nullptr;
static std::string g_shaders_dir;

static const char* GetProfileName( r_GLSLVersion::Profile profile )
{
	switch( profile )
	{
		case r_GLSLVersion::Profile::Core: return "core";
		case r_GLSLVersion::Profile::Compatibility: return "compatibility";
		case r_GLSLVersion::Profile::None: return "";
	};
	return "";
}

static std::string LoadShaderFile( const std::string& file_name )
{
	std::string result;

	std::string full_file_name= g_shaders_dir + file_name;

	std::FILE* file= std::fopen( full_file_name.data(), "rb" );
	if( file == nullptr )
	{
		if( g_shader_loading_log_callback )
			g_shader_loading_log_callback( ( full_file_name + ": Error, can not load file." ).data() );
		return result;
	}

	std::fseek( file, 0, SEEK_END );
	unsigned int file_size= std::ftell( file );
	std::fseek( file, 0, SEEK_SET );

	result.resize( file_size );

	unsigned int pos= 0;
	do
	{
		pos+= std::fread( const_cast<char*>(result.data()) + pos, 1, file_size - pos, file );
	} while ( pos < file_size && !std::feof( file ) );

	std::fclose( file );

	return result;
}

static void LogError( const std::string& file_name, unsigned int line, const char* error_message )
{
	if( g_shader_loading_log_callback )
	{
		char line_num[32];
		std::snprintf( line_num, sizeof(line_num), "%d", line );
		g_shader_loading_log_callback( ( file_name + ":" + line_num + ": " + error_message ).data() );
	}
}

static std::string LoadShader_r( const std::string& file_name )
{
	const std::string src_text= LoadShaderFile( file_name );
	std::string result;
	result.reserve( src_text.size() );

	bool prev_symbol_is_newline= true;
	unsigned int line= 1;

	result+= "/* start file \"";
	result+= file_name;
	result+= "\" */\n";

	for( unsigned int i= 0; i < src_text.size(); )
	{
		char c= src_text[i];

		static const char c_include_derective[]= "#include";
		if( prev_symbol_is_newline &&
			c == '#' &&
			i + sizeof(c_include_derective) - 1 <= src_text.size() &&
			std::memcmp(
				src_text.data() + i,
				c_include_derective,
				sizeof(c_include_derective) - 1 ) == 0 )
		{
			i+= sizeof(c_include_derective) - 1;

			while( i < src_text.size() && src_text[i] != '"' )
			{
				if( !std::isspace( src_text[i] ) )
				{
					LogError( file_name, line, "Error, unexpected character after #include." );
					return result;
				}
				i++;
			}
			if( i == src_text.size() )
			{
				LogError( file_name, line, "Error, unexpected end of file." );
				return result;
			}
			i++;

			const char* file_name_start= src_text.data() + i;
			while( i < src_text.size() && src_text[i] != '"' ) i++;
			if( i == src_text.size() )
			{
				LogError( file_name, line, "Error, unexpected end of file." );
				return result;
			}

			const char* file_name_end= src_text.data() + i;
			i++;

			result+= LoadShader_r( std::string( file_name_start, file_name_end ) );
		}
		else
		{
			i++;
			result.push_back(c);

			if( c == '\n' )
			{
				line++;
				prev_symbol_is_newline= true;
			}
			else prev_symbol_is_newline= false;
		}
	}

	result+= "/* end file \"";
	result+= file_name;
	result+= "\" */";

	return result;
}

unsigned int r_GLSLVersion::OpenGLVersionToGLSLVersionNumber(
	unsigned int major_ogl_version,
	unsigned int minor_ogl_version )
{
	if( major_ogl_version < 2 )
		return 0;

	if( major_ogl_version == 2 )
	{
		if( minor_ogl_version == 0 ) return v110;
		if( minor_ogl_version == 1 ) return v120;
		return v120;
	}
	if( major_ogl_version == 3 )
	{
		if( minor_ogl_version == 0 ) return v130;
		if( minor_ogl_version == 1 ) return v140;
		if( minor_ogl_version == 2 ) return v150;
		if( minor_ogl_version == 3 ) return v330;
		return v330;
	}
	if( major_ogl_version == 4 )
	{
		if( minor_ogl_version == 0 ) return v400;
		if( minor_ogl_version == 1 ) return v410;
		if( minor_ogl_version == 2 ) return v420;
		if( minor_ogl_version == 3 ) return v430;
		if( minor_ogl_version == 4 ) return v440;
		if( minor_ogl_version == 5 ) return v450;
	}

	// Try get something for unknown version abowe 3
	return major_ogl_version * 100 + minor_ogl_version * 10;
}

void rSetShaderLoadingLogCallback( ShaderLoadingLogCallback callback )
{
	g_shader_loading_log_callback= callback;
}

void rSetShadersDir( std::string dir_name )
{
	g_shaders_dir= std::move(dir_name);

	// Normalize path.
	while(
		!g_shaders_dir.empty() &&
		(g_shaders_dir.back() == '/' || g_shaders_dir.back() == '\\') )
		g_shaders_dir.pop_back();

	if( !g_shaders_dir.empty() )
		g_shaders_dir.push_back( '/' );
}

std::string rLoadShader(
	const std::string& file_name,
	const r_GLSLVersion& version,
	const std::vector<std::string>& defines )
{
	std::string result;

	char version_str[64];

	std::snprintf(
		version_str, sizeof(version_str),
		"#version %d %s\n",
		version.version_number, GetProfileName( version.profile ) );

	result+= version_str;

	for( const std::string& define : defines )
	{
		result+= "#define ";
		result+= define;
		result+= "\n";
	}

	result+= LoadShader_r( file_name );

	return result;
}
