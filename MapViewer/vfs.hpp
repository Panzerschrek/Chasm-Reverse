#pragma once
#include <cstdio>
#include <vector>

namespace ChasmReverse
{

// Virtual file system
class Vfs final
{
public:
	typedef std::vector<unsigned char> FileContent;

	explicit Vfs( const char* archive_file_name );
	~Vfs();

	FileContent ReadFile( const char* file_name ) const;

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

	VirtualFiles virtual_files_;
};

} // namespace ChasmReverse
