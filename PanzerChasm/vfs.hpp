#pragma once
#include <cstdio>
#include <memory>
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
		char name[12]; // null terminated for names with length (1-11)
		unsigned int offset;
		unsigned int size;
	};

	typedef std::vector<VirtualFile> VirtualFiles;

private:
	std::FILE* const archive_file_;
	const std::string addon_path_;

	VirtualFiles virtual_files_;
};

} // namespace PanzerChasm
