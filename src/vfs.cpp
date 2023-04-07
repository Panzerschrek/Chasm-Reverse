#include <algorithm>
#include <cctype>
#include <cstring>

#include "common/files.hpp"
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

static std::string ToUpper( const std::string& s )
{
	std::string r= s;
	for( char& c : r )
		c= std::toupper(c);
	return r;
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

Vfs::VurtualFileName::VurtualFileName( const char* const in_text )
{
	size_t i= 0u;
	while( i < sizeof(text) && in_text[i] != '\0' )
	{
		text[i]= in_text[i];
		++i;
	}

	if( i < sizeof(text) )
		text[i]= '\0';
}

Vfs::VurtualFileName::VurtualFileName( const char* const in_text, const size_t size )
{
	std::memcpy( &text, in_text, std::min( size, sizeof(text) ) );
	if( size < sizeof(text) )
		text[size]= '\0';
}

bool Vfs::VurtualFileName::operator==( const VurtualFileName& other ) const
{
	return StringEquals( this->text, other.text, sizeof(text) );
}

size_t Vfs::VurtualFileNameHasher::operator()( const VurtualFileName& name ) const
{
	// djb2 hash
	size_t hash= 5381u;
	for( size_t i= 0u; i < sizeof(name.text) && name.text[i] != '\0'; ++i )
		hash= hash * 33u + size_t(std::tolower(name.text[i]));

	return hash;
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
		VirtualFile file;
		std::memcpy( &file.size  , &file_info_packed.size  , sizeof(unsigned int) );
		std::memcpy( &file.offset, &file_info_packed.offset, sizeof(unsigned int) );

		virtual_files_[ VurtualFileName( file_info_packed.name, file_info_packed.name_length ) ]= file;
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
	const char* const file_name= ExtractFileName( file_path );
	if( file_name[0] == '\0' )
	{
		// Do not load files with empty path.
		out_file_content.clear();
		return;
	}

	// Try read from real file system.
	if( !addon_path_.empty() )
	{
		std::string fs_file_path= addon_path_ + ToUpper(file_path); // Use ToUpper, because files in addons are in upper case.
		std::replace(fs_file_path.begin(), fs_file_path.end(), '\\', '/' ); // Change shitty DOS/Windows path separators to universal windows/unix separators.

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

	const auto it= virtual_files_.find( VurtualFileName( file_name ) );
	if( it != virtual_files_.end() )
	{
		const VirtualFile& file= it->second;
		out_file_content.resize( file.size );
		std::fseek( archive_file_, file.offset, SEEK_SET );
		FileRead( archive_file_, out_file_content.data(), out_file_content.size() );

		return;
	}

	out_file_content.clear();
}

} // namespace PanzerChasm
