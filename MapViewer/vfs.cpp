#include <cstring>
#include <iostream>

#include "../Common/files.hpp"

#include "vfs.hpp"

namespace ChasmReverse
{

#pragma pack(push, 1)
struct FileInfoPacked
{
	unsigned char name_length;
	char name[ 12 ]; // without end null
	unsigned int size;
	unsigned int offset;
};
#pragma pack(pop)

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

	return true;
}

Vfs::Vfs( const char* archive_file_name )
	: archive_file_( std::fopen( archive_file_name, "rb" ) )
{
	if( archive_file_ == nullptr )
	{
		std::cout << "Could not open file \"" << archive_file_name << "\"" << std::endl;
		return;
	}

	char header[4];
	FileRead( archive_file_, header, sizeof(header) );
	if( std::strncmp( header, "CSid", sizeof(header) ) != 0 )
	{
		std::cout << "File \"" << archive_file_name << "\" is not \"Chasm: The Rift\" archive" << std::endl;
		return;
	}

	unsigned short files_in_archive_count;
	FileRead( archive_file_, &files_in_archive_count, sizeof(files_in_archive_count) );

	std::vector<FileInfoPacked> files_info_packed( files_in_archive_count );
	FileRead( archive_file_, files_info_packed.data(), files_info_packed.size() * sizeof(FileInfoPacked ) );

	virtual_files_.reserve( files_in_archive_count );

	for( const FileInfoPacked& file_info_packed : files_info_packed )
	{
		virtual_files_.emplace_back();
		VirtualFile& file= virtual_files_.back();

		std::memcpy( file.name, file_info_packed.name, file_info_packed.name_length );
		if( file_info_packed.name_length < sizeof(file.name) )
			file.name[ file_info_packed.name_length ]= '\0';

		std::memcpy( &file.size  , &file_info_packed.size  , sizeof(unsigned int) );
		std::memcpy( &file.offset, &file_info_packed.offset, sizeof(unsigned int) );
	}
}

Vfs::~Vfs()
{
	if( archive_file_ != nullptr )
		std::fclose( archive_file_ );
}

Vfs::FileContent Vfs::ReadFile( const char* file_name )
{
	FileContent result;

	// TODO - sort virtual_files_ and use binary search
	for( const VirtualFile& file : virtual_files_ )
	{
		if( !StringEquals( file.name, file_name, 12 ) )
			continue;

		result.resize( file.size );
		std::fseek( archive_file_, file.offset, SEEK_SET );
		FileRead( archive_file_, result.data(), result.size() );

		break;
	}

	return result;
}

} // namespace ChasmReverse
