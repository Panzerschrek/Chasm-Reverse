#include <cstring>

#include "../Common/files.hpp"
using namespace ChasmReverse;

#include "log.hpp"

#include "vfs.hpp"

namespace PanzerChasm
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

	return i == max_length || s0[i] == s1[i];
}

static const char* ExtractFileName( const char* const file_path )
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

static std::string PrepareAddonPath( const char* const addon_path )
{
	if( addon_path == nullptr )
		return "";

	std::string result= addon_path;
	if( !result.empty() )
	{
		if( !( result.back() == '/' || result.back() == '\\' ) )
			result.push_back( '/' );
	}
	return result;
}

Vfs::Vfs(
	const char* archive_file_name,
	const char* const addon_path )
	: archive_file_( std::fopen( archive_file_name, "rb" ) )
	, addon_path_( PrepareAddonPath( addon_path ) )
{
	if( archive_file_ == nullptr )
	{
		Log::FatalError( "Could not open file \"", archive_file_name, "\"" );
		return;
	}

	char header[4];
	FileRead( archive_file_, header, sizeof(header) );
	if( std::strncmp( header, "CSid", sizeof(header) ) != 0 )
	{
		Log::FatalError( "File \"", archive_file_name, "\" is not \"Chasm: The Rift\" archive" );
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

Vfs::FileContent Vfs::ReadFile( const char* const file_path ) const
{
	FileContent result;
	ReadFile( file_path, result );
	return result;
}

void Vfs::ReadFile( const char* const file_path, FileContent& out_file_content ) const
{
	// Try read from real file system.
	if( !addon_path_.empty() )
	{
		const std::string fs_file_path= addon_path_ + file_path;
		std::FILE* const fs_file= std::fopen( fs_file_path.c_str(), "rb" );
		if( fs_file != nullptr )
		{
			std::fseek( fs_file, 0, SEEK_END );
			const unsigned int file_size= std::ftell( fs_file );
			std::fseek( fs_file, 0, SEEK_SET );

			out_file_content.resize( file_size );
			FileRead( fs_file, out_file_content.data(), file_size );

			std::fclose( fs_file );
			return;
		}
	}

	const char* const file_name= ExtractFileName( file_path );

	// TODO - sort virtual_files_ and use binary search
	for( const VirtualFile& file : virtual_files_ )
	{
		if( !StringEquals( file.name, file_name, 12 ) )
			continue;

		out_file_content.resize( file.size );
		std::fseek( archive_file_, file.offset, SEEK_SET );
		FileRead( archive_file_, out_file_content.data(), out_file_content.size() );

		return;
	}

	out_file_content.clear();
}

} // namespace PanzerChasm
