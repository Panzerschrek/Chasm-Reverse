#pragma once

namespace PanzerChasm
{

namespace FontParams
{

const int letter_place_width= 16;
const int letter_place_height= 10;
const int letter_u_offset= 2;
const int letter_v_offset= 1;
const int letter_height= 9;
const int space_width= 5;

const unsigned int atlas_width = 16u * letter_place_width ;
const unsigned int atlas_height= 16u * letter_place_height;

const unsigned int colors_variations= 4u;

// Indeces of colors, used in font as inner for letters.
const unsigned char start_inner_color= 4u;
const unsigned char end_inner_color= 16u;

const int color_shifts[ colors_variations ]=
{
	  0, // white
	 38, // dark-yellow
	176, // golden
	194, // dark-yellow with green
};

const char font_file_name[]= "FONT256.CEL";

} // namespace FontParams

// Output - 256 widths
void CalculateLettersWidth(
	const unsigned char* texture_data,
	unsigned char* out_width );

} // namespace PanzerChasm
