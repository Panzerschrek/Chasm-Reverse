#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

#pragma pack(push, 1)

struct Header
{
	char id[2];
	unsigned short width;
	unsigned short height;
	unsigned char reserved[32-6];
};

static_assert( sizeof(Header) == 32, "Invalid size" );

struct TGAHeader
{
	unsigned char id_length;
	unsigned char colormap_type;
	unsigned char image_type;

	unsigned short colormap_index;
	unsigned short colormap_length;
	unsigned char colormap_size;

	unsigned short x_origin;
	unsigned short y_origin;
	unsigned short width ;
	unsigned short height;

	unsigned char pixel_bits;
	unsigned char attributes;
};

static_assert( sizeof(TGAHeader) == 18, "Invalid size" );

#pragma pack(pop)

static void FileRead( std::FILE* const file, void* buffer, const unsigned int size )
{
	unsigned int read_total= 0u;

	do
	{
		const int read= std::fread( buffer, 1, size, file );
		if( read <= 0 )
			break;

		read_total+= read;
	} while( read_total < size );
}

static void FileWrite( std::FILE* const file, const void* buffer, const unsigned int size )
{
	unsigned int write_total= 0u;

	do
	{
		const int write= std::fwrite( buffer, 1, size, file );
		if( write <= 0 )
			break;

		write_total+= write;
	} while( write_total < size );
}

void WriteTGA(
	const unsigned short width, const unsigned short height,
	const unsigned char* const data,
	const unsigned char* const palette,
	const char* const file_name )
{
	TGAHeader tga;

	tga.id_length= 0;
	tga.colormap_type= 1; // image with colormap
	tga.image_type= 1; // image with palette without compression

	tga.colormap_index= 0;
	tga.colormap_length= 256;
	tga.colormap_size= 32;

	tga.x_origin= 0;
	tga.y_origin= 0;
	tga.width = width ;
	tga.height= height;

	tga.pixel_bits= 8;
	tga.attributes= 1 << 5; // vertical flip flag
	tga.attributes|= 8; // bits in alpha-channell

	std::FILE* file= std::fopen( file_name, "wb" );
	if( file == nullptr )
	{
		std::cout << "Could not create file \"" << file_name << "\"" << std::endl;
		return;
	}

	FileWrite( file, &tga, sizeof(tga) );

	unsigned char palette_rb_swapped[ 256 * 4 ];
	for( unsigned int i= 0; i < 256; i++ )
	{
		palette_rb_swapped[ i * 4 + 0 ]= palette[ i * 3 + 2 ];
		palette_rb_swapped[ i * 4 + 1 ]= palette[ i * 3 + 1 ];
		palette_rb_swapped[ i * 4 + 2 ]= palette[ i * 3 + 0 ];
		palette_rb_swapped[ i * 4 + 3 ]= 255;
	}
	palette_rb_swapped[255 * 4 + 3]= 0;


	FileWrite( file, palette_rb_swapped, sizeof(palette_rb_swapped) );
	FileWrite( file, data, width * height );

	std::fclose( file );
}

static void ShiftPalette( unsigned char* const palette )
{
	for( unsigned int i= 0; i < 768; i++ )
	{
		const int shifted= palette[i] << 2;
		palette[i]= std::min( shifted, 255 );
	}
}

int main( const int argc, const char* const argv[] )
{
	const char* in_file_name= "";
	const char* out_file_name= "";

	for( int i= 1; i < argc; i++ )
	{
		const char* const arg= argv[i];
		if( std::strcmp( arg, "-i" ) == 0 )
		{
			if( i < argc - 1 )
				in_file_name= argv[ i + 1 ];
			else
				std::cout << "Error, expected file name, after -i" << std::endl;
		}
		else if( std::strcmp( arg, "-o" ) == 0 )
		{
			if( i < argc - 1 )
				out_file_name= argv[ i + 1 ];
			else
				std::cout << "Error, expected file name, after -o" << std::endl;
		}
	}

	if( std::strlen( in_file_name ) == 0 )
	{
		std::cout << "Error, no input file" << std::endl;
		return -1;
	}

	char out_file_name_buffer[ 1024 ];
	if( std::strlen( out_file_name ) == 0 )
	{
		const int in_file_name_len= std::strlen( in_file_name );
		int i= in_file_name_len;

		while( i >= 0 && in_file_name[i] != '.' )
			i--;

		if( i >= 0 )
		{
			std::memcpy( out_file_name_buffer, in_file_name, i + 1 );
			std::memcpy( out_file_name_buffer + i + 1, "tga", 4 );
		}
		else
		{
			std::memcpy( out_file_name_buffer, in_file_name, in_file_name_len );
			std::memcpy( out_file_name_buffer + in_file_name_len, ".tga", 5 );
		}

		out_file_name= out_file_name_buffer;
	}

	std::FILE* file= std::fopen( in_file_name, "rb" );
	if( file == nullptr )
	{
		std::cout << "Could not read file \"" << in_file_name << "\"" << std::endl;
		return -1;
	}

	Header header;
	FileRead( file, &header, sizeof(header) );

	unsigned char palette[768];
	FileRead( file, palette, sizeof(palette) );
	ShiftPalette( palette );

	std::vector<unsigned char> file_data( header.width * header.height );
	FileRead( file, file_data.data(), file_data.size() );

	std::fclose( file );

	WriteTGA( header.width, header.height, file_data.data(), palette, out_file_name );
}
