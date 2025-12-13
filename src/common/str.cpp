#include "str.hpp"

const char* ExtractExtension( const char* const file_path )
{
	unsigned int pos= std::strlen( file_path );

	while( pos > 0u && file_path[ pos ] != '.' )
		pos--;

	return file_path + pos + 1u;
}

std::filesystem::path remove_extension( const std::filesystem::path& path )
{
	if( path == "." || path == ".." ) return path;

	std::filesystem::path dst = path.stem();

	while(!dst.extension().empty()) dst = dst.stem();

	return path.parent_path() / dst;
}

const char* ExtractFileName( const char* const file_path )
{
	const char* file_name_pos= file_path;

	const char* str= file_path;
	while( *str != '\0' )
	{
		if( *str == '/' || *str == '\\' )
			file_name_pos= str + 1u;

		str++;
	}

	return file_name_pos;
}

std::string ToUpper( const std::string& s )
{
	std::string r= s;
	for( char& c : r )
		c= isalnum(c) ? std::toupper(c) : c;
	return r;
}

std::string ToLower( const std::string& s )
{
	std::string r= s;
	for( char& c : r )
		c= isalnum(c) ? std::tolower(c) : c;
	return r;
}

