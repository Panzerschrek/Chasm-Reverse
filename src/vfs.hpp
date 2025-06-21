#pragma once
#include <string>
#include <cstdio>
#include <memory>
#include <unordered_map>
#include <vector>
#include <system_error>
#include <filesystem>
#include <physfs.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>
#include "common/files.hpp"
using namespace ChasmReverse;

#include "log.hpp"


namespace PanzerChasm
{


class Vfs final
{
public:
	typedef std::vector<unsigned char> FileContent;
	enum flags
	{
		NIL = 0 << 0,
		CSM = 1 << 0,
		TAR = 1 << 1,
		DIR = 1 << 2,
	};
	struct entry
	{
		std::filesystem::path path;
		enum flags type;
		bool lfn;
	};
	struct entry archive_;
	struct entry addon_;

	Vfs( const std::filesystem::path& archive_file, const std::filesystem::path& addon_path );
	~Vfs();
	Vfs::FileContent ReadFile( const std::filesystem::path& file_path ) const;
	void ReadFile( const std::filesystem::path& file_path, FileContent& out_file_content ) const;
private:
	int Type( const std::filesystem::path& file ) const; 
	int SupportedFormats() const;
	int formats_;
	std::map<const std::filesystem::path, int> entries_;
};

} // namespace PanzerChasm
