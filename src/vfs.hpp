#pragma once
#include <string>
#include <cstdio>
#include <memory>
#include <unordered_map>
#include <vector>

namespace PanzerChasm
{

// Virtual file system
class Vfs final
{
public:
	typedef std::vector<unsigned char> FileContent;

	explicit Vfs( const char* archive_file_name, const char* addon_path= nullptr );
	~Vfs();

	FileContent ReadFile( const char* file_path ) const;
	void ReadFile( const char* file_path, FileContent& out_file_content ) const;

private:
	struct VirtualFile
	{
		unsigned int offset;
		unsigned int size;
	};

	struct VurtualFileName
	{
		char text[12u]; // null terminated for names with length (1-11)

		VurtualFileName( const char* in_text );
		VurtualFileName( const char* in_text, size_t size );
		bool operator==( const VurtualFileName& other ) const;
	};

	struct VurtualFileNameHasher
	{
		size_t operator()( const VurtualFileName& name ) const;
	};

	typedef std::unordered_map< VurtualFileName, VirtualFile, VurtualFileNameHasher > VirtualFiles;

private:
	std::FILE* const archive_file_;
	const std::string addon_path_;

	VirtualFiles virtual_files_;
};

} // namespace PanzerChasm
