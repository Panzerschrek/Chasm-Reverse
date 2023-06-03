#pragma once
#include <cstdio>
#include <string>
// Include OS-dependend stuff for "mkdir".
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace ChasmReverse
{

void FileRead( std::FILE* const file, void* buffer, const unsigned int size );
void FileWrite( std::FILE* const file, const void* buffer, const unsigned int size );
const char* ExtractExtension( const char* const file_path );


} // namespace ChasmReverse

template<class T>
T dir_name(T const& path, T const& delims = "/\\")
{
	return path.substr(0, path.find_last_of(delims));
}

template<class T>
T base_name(T const& path, T const& delims = "/\\")
{
	return path.substr(path.find_last_of(delims) + 1);
}

template<class T>
T remove_extension(T const& filename)
{
	typename T::size_type const p(base_name<std::string>(filename).find_last_of('.'));
	return ((p > 0 && p != T::npos) ? dir_name<std::string>(filename) + "/" + base_name<std::string>(filename).substr(0, p) : filename);
}

template<class T>
T extract_extension(T const& filename)
{
	typename T::size_type const p(base_name<std::string>(filename).find_last_of('.'));
	return ((p > 0 && p != T::npos) ? dir_name<std::string>(filename) + "/" + base_name<std::string>(filename).substr(p, T::npos) : filename);
}

bool create_directory(const std::string& directory);
