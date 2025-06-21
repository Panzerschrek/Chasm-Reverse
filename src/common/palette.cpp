#include <iostream>

#include "files.hpp"

#include "palette.hpp"

namespace ChasmReverse
{

void LoadPalette( Palette& out_palette )
{
	const char* const palette_file_name= "CHASM2.PAL";

	std::FILE* const file= std::fopen( palette_file_name, "rb" );

	if( file == nullptr )
	{
		std::cout << "Could not read file \"" << palette_file_name << "\"" << std::endl;
		return;
	}
	std::cout << "FOOO" << std::endl;
	FileRead( file, out_palette.data(), out_palette.size() );

	std::fclose( file );

	for( unsigned char& palette_element : out_palette )
		palette_element <<= 2u;
}

} // namespace ChasmReverse
