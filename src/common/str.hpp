#pragma once

#ifdef _MSC_VER
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#ifdef _stristr
#define strcasestr _stristr
#define strncasestr _strnistr
#else
#include <shlwapi.h>
inline char* strcasestr(const char* haystack, const char* needle)
{
	return StrStrI(needle, haystack);
}

inline char* strncasestr(const char* haystack, const char* needle, size_t n)
{
	return StrStrNI(needle, haystack, n);
}

#endif
#else
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif
#include <cstring>
#include <cstdlib>
#include <filesystem>

#define GetSubstring strcasestr
constexpr bool StringEquals( const char* s0, const char* s1, size_t n = 0)
{
	return n == 0 ? ( ::strcasecmp( s0, s1 ) == 0 ) : ( ::strncasecmp( s0, s1, n ) == 0 );
}

const char* ExtractFileName( const char* const file_path );
const char* ExtractExtension( const char* const file_path );
std::filesystem::path remove_extension( const std::filesystem::path& path );
std::string ToUpper( const std::string& s );
std::string ToLower( const std::string& s );
/*
// Own replacement for nonstandard strncasecmp
static bool StringEquals( const char* const s0, const char* const s1, const unsigned int max_length )
{
	unsigned int i= 0;
	while( s0[i] != '\0' && s1[i] != '\0' && i < max_length )
	{
		if( std::tolower( s0[i] ) != std::tolower( s1[i] ) )
			return false;
		i++;
	}

	return i == max_length || s0[i] == s1[i];
}
*/

