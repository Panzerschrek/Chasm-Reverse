#pragma once
#include "i_menu_drawer.hpp"

namespace PanzerChasm
{

namespace MenuParams
{

const char* const menu_pictures[ size_t(IMenuDrawer::MenuPicture::PicturesCount) ]=
{
	"M_MAIN.CEL",
	"M_NEW.CEL",
	"M_NETWRK.CEL",
};

const char tile_picture[]= "M_TILE1.CEL";
const char loading_picture[]= "COMMON/LOADING.CEL";
const char background_picture[]= "GROUND.CEL";
const char pause_picture[]= "M_PAUSE.CEL";

const unsigned int menu_pictures_shifts_count= 3u;
const int menu_pictures_shifts[ menu_pictures_shifts_count ]=
{
	 0,
	64, // golden
	96, // dark-yellow
};

// Indeces of colors, used in ,enu pictures as inner for letters.
const unsigned char start_inner_color= 7u * 16u;
const unsigned char end_inner_color= 8u * 16u;

const unsigned int menu_picture_row_height= 20u;
const unsigned int menu_picture_horizontal_border= 4u;

const unsigned int menu_border= 3u;
const unsigned int menu_caption= 10u;

} // namespace MenuParams

unsigned int CalculateMenuScale( const Size2& viewport_size );
unsigned int CalculateConsoleScale( const Size2& viewport_size );

} // namespace PanzerChasm
