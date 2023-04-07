#pragma once
#include <string>
#include <vector>

struct r_GLSLVersion
{
	enum KnowmNumbers : unsigned int
	{
		v100= 100,
		v110= 110,
		v120= 120,
		v130= 130,
		v140= 140,
		v150= 150,
		v330= 330,
		v400= 400,
		v410= 410,
		v420= 420,
		v430= 430,
		v440= 440,
		v450= 450,
	};

	enum class Profile
	{
		Core,
		Compatibility,
		None,
	};

	static unsigned int OpenGLVersionToGLSLVersionNumber( unsigned int major_ogl_version, unsigned int minor_ogl_version );

	r_GLSLVersion( unsigned int in_version_number= 0, Profile in_profile= Profile::None )
		: version_number(in_version_number), profile( in_profile )
	{}

	unsigned int version_number;
	Profile profile;
};

typedef void (*ShaderLoadingLogCallback)( const char* log_data );

void rSetShaderLoadingLogCallback( ShaderLoadingLogCallback callback );

// Set directory ,relative or absolute, for shades loading.
void rSetShadersDir( std::string dir_name );

// Loads shader, adds version directive. Recursive load included shaders (using #include).
std::string rLoadShader(
	const std::string& file_name,
	const r_GLSLVersion& version,
	const std::vector<std::string>& defines= std::vector<std::string>() );

