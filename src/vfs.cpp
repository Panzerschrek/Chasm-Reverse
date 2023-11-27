#include "vfs.hpp"

namespace PanzerChasm {

	int Vfs::SupportedFormats() const
	{
		if( PHYSFS_isInit() == 0 )
		{
			if( PHYSFS_init( "" ) == 0 )
			{
				Log::FatalError( "PHYSFS_init() failed!\n reason %s.\n", PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode() ) );
				std::error_code ec;
				throw std::filesystem::filesystem_error(std::string( "Could not initialize PhysFS!" ), ec);
			}
		}

		const PHYSFS_ArchiveInfo **rc = PHYSFS_supportedArchiveTypes();
		const PHYSFS_ArchiveInfo **i;
		int supported = DIR;

		if( *rc == nullptr )
			printf( "PHYSFS BIN/TAR format not supported!\n" );
		else
		{
			for(i = rc; *i != nullptr; i++)
			{
				if( memcmp((*i)->extension, "TAR", 3) == 0 )
					supported |= TAR;
				if( memcmp((*i)->extension, "BIN", 3) == 0 )
					supported |= CSM;
			}
		}
		return supported;
	}

	int Vfs::Type( const std::filesystem::path& path ) const
	{
		static const char* CSM_MAGIC = "CSid";
		static const char* TAR_MAGIC = "ustar  ";	/* 7 chars and a null */
		static const ssize_t TAR_MAGIC_OFFSET = 257;
		static const ssize_t TAR_BLOCK_SIZE = 512;
		static const ssize_t CSM_BLOCK_SIZE = 4;
		int res = NIL;
		if( !path.empty() )
		{ 
			if( !std::filesystem::is_directory( path ) )
			{
				char buf[TAR_BLOCK_SIZE + 1] = { '\0' };
				std::ifstream is( path, std::ios_base::binary );
				is.read( buf, TAR_BLOCK_SIZE );
				if( is.tellg() == TAR_BLOCK_SIZE )
				{
					if( strncmp( buf, CSM_MAGIC, CSM_BLOCK_SIZE ) == 0 )
					{
						res = CSM;
					}
					else if( strncmp(&buf[TAR_MAGIC_OFFSET], TAR_MAGIC, CSM_BLOCK_SIZE + 1 ) == 0 )
						res = TAR;
				}
				is.close();
			}
			else
			{
				res = DIR;
			}
		}
		return res;
	}

	Vfs::Vfs( const std::filesystem::path& archive_file, const std::filesystem::path& addon_path )
	{
		archive_.path = archive_file;
		archive_.type = (enum flags)Type( archive_.path );
		addon_.path   = addon_path;
		addon_.type   = (enum flags)Type( addon_.path );
		formats_ = SupportedFormats();

		if( !archive_.path.empty() )
		{
			if( (PHYSFS_mount( archive_.path.c_str(), "/", 0 ) == 0) )
			{
				Log::FatalError( "Could not open archice file \"", archive_.path, "\"" );
				PHYSFS_deinit();
				std::error_code ec;
				throw std::filesystem::filesystem_error( std::string( "Could not initialize PhysFS!" ), ec );
			}
			Log::Info( archive_.type == TAR ? "TAR" : "BIN", " ", archive_.path );
		}

		if( !addon_.path.empty() )
		{
			if( ( PHYSFS_mount( addon_.path.c_str(), "/", 0 ) == 0) )
			{
				Log::FatalError( "Could not open addon path \"", addon_.path, "\"" );
				PHYSFS_deinit();
				std::error_code ec;
				throw std::filesystem::filesystem_error( std::string( "Could not initialize PhysFS!" ), ec);
			}
			Log::Info( addon_.type == DIR ? "DIR" : "NIL", " ", addon_.path );
		}
	}

	Vfs::~Vfs()
	{
		PHYSFS_deinit();
	}

	Vfs::FileContent Vfs::ReadFile( const std::filesystem::path& file_path ) const
	{
		FileContent result;
		ReadFile( file_path, result );
		return result;
	}

	void Vfs::ReadFile( const std::filesystem::path& file_path, FileContent& out_file_content ) const
	{	
		std::string file_path_str = file_path.native();
		std::replace(file_path_str.begin(), file_path_str.end(), '\\', '/');
		std::filesystem::path loc( ToUpper( file_path_str ) );

		if( !loc.empty() )
		{
			std::error_code ec;
			if( PHYSFS_isInit() == 0 )
			{
				throw std::filesystem::filesystem_error( std::string( "Could not initialize PhysFS!" ), ec );
			}

			if( !addon_.path.empty() && addon_.type == DIR )
			{
				PHYSFS_File *f = PHYSFS_openRead( loc.c_str() );
				if( f != nullptr )
				{
					PHYSFS_sint64 file_size= PHYSFS_fileLength( f );
					out_file_content.resize( file_size );
					PHYSFS_sint64 read = PHYSFS_readBytes( f, out_file_content.data(), out_file_content.size() );
					if( read == out_file_content.size() )
					{
						PHYSFS_flush( f );
						PHYSFS_close( f );
						return;
					}
					out_file_content.clear();
				}
				PHYSFS_close(f);
			}

			if( !archive_.path.empty() && archive_.type == CSM )
			{
				PHYSFS_File *f = PHYSFS_openRead( loc.filename().c_str()  );
				if( f != nullptr )
				{
					PHYSFS_sint64 file_size = PHYSFS_fileLength( f );
					out_file_content.resize( file_size );
					PHYSFS_sint64 read = PHYSFS_readBytes( f, out_file_content.data(), out_file_content.size() );
					if( read == out_file_content.size() )
					{
						PHYSFS_flush( f );
						PHYSFS_close( f );
						return;
					}
					out_file_content.clear();
				}
				PHYSFS_close( f );
			}

			if( !archive_.path.empty() && archive_.type == TAR )
			{
				loc = ToLower( loc.native() );
				if( loc.filename().extension() == ".raw" || loc.filename().extension() == ".sfx" || loc.filename().extension() == ".pcm" )
				{
					if( !(loc.filename() == "aproc.sfx" || loc.filename() == "wswap.sfx" || loc.filename() == "thund3.raw" || loc.filename() == "volc.raw" || loc.filename() == "step02.raw" ) )
						loc = "sound" / loc.filename().replace_extension( ".wav" );
				}
				else if( loc.filename().extension() == ".wav" )
					loc = "sound" / loc.filename();
				else if( loc.filename().extension() == ".cel" )
					loc = "texture" / loc.filename(); 
				else if( loc.parent_path() == "models" )
					loc = "model" / loc.filename(); 
				else if( loc.filename().extension() == ".car" )
					loc = "monster" / loc.filename();
				else if( loc.parent_path() == "ani" || loc.parent_path() == "ani/weapon" || loc.parent_path() == "ani/weapon" )
					loc = "model" / loc.filename();
				else if( loc.stem() == "floors" || loc.stem() == "resource" || loc.stem() == "process" || loc.stem() == "map" || loc.stem() == "script" )
					loc = "map" / loc.filename();
				else if( loc.filename().extension() == ".obj" )
				{
					if( loc.parent_path() != "monster" )
						loc = "sprite_effects" / loc.filename();
				}
				else if( loc.filename().extension() == ".ani" )
					loc = "model" / loc.filename();
				if( loc.filename().extension() == ".3o" )
					loc = "model" / loc.filename();
				else if( loc.filename().extension() == ".gltf" )
				{
					if( loc.parent_path() != "monster" )
						loc = "model" / loc.filename();
				} 

				if( !loc.filename().has_extension() )
				{
					std::filesystem::path oldloc = loc;
					if( loc.filename() == "faust" )
						loc = "monster" / loc.filename().replace_filename( "faust1.car" );
					if( loc.filename() == "vent1" )
						loc = "model" / loc.filename().replace_filename( "vent1.gltf" );
					Log::Warning( "Warning: ", oldloc, " -> ", loc, " renamed!" );
				}

				char* path = (char*)loc.c_str();
				PHYSFS_File *f = PHYSFS_openRead( ( path ) );
				if( f != nullptr )
				{
					PHYSFS_sint64 file_size = PHYSFS_fileLength( f );
					out_file_content.resize( file_size );
					PHYSFS_sint64 read = PHYSFS_readBytes( f, out_file_content.data(), out_file_content.size() );
					if( read == out_file_content.size() )
					{
						PHYSFS_flush( f );
						PHYSFS_close( f );
						return;
					}
					out_file_content.clear();
				}
				PHYSFS_close( f );
			}
		}
	}
};
