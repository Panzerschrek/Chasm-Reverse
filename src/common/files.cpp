#include "files.hpp"
#include <cstring>

namespace ChasmReverse
{

void FileRead( std::FILE* const file, void* buffer, const unsigned int size )
{
	unsigned int read_total= 0u;

	do
	{
		const int read= std::fread( static_cast<char*>(buffer) + read_total, 1, size - read_total, file );
		if( read <= 0 )
			break;

		read_total+= read;
	} while( read_total < size );
}

void FileWrite( std::FILE* const file, const void* buffer, const unsigned int size )
{
	unsigned int write_total= 0u;

	do
	{
		const int write= std::fwrite( static_cast<const char*>(buffer) + write_total, 1, size - write_total, file );
		if( write <= 0 )
			break;

		write_total+= write;
	} while( write_total < size );
}

const char* ExtractExtension( const char* const file_path )
{
	unsigned int pos= std::strlen( file_path );

	while( pos > 0u && file_path[ pos ] != '.' )
		pos--;

	return file_path + pos + 1u;
}

} // namespace ChasmReverse

bool create_directory(const std::string& directory)
{
#ifdef _WIN32
	return (_mkdir( directory.c_str() ) == 0 ? true : false);
#else
	return (mkdir( directory.c_str(), 0777 ) == 0 ? true : false);
#endif
}
