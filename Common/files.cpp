#include "files.hpp"

namespace ChasmReverse
{

void FileRead( std::FILE* const file, void* buffer, const unsigned int size )
{
	unsigned int read_total= 0u;

	do
	{
		const int read= std::fread( buffer + read_total, 1, size - read_total, file );
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
		const int write= std::fwrite( buffer + write_total, 1, size - write_total, file );
		if( write <= 0 )
			break;

		write_total+= write;
	} while( write_total < size );
}


} // namespace ChasmReverse
