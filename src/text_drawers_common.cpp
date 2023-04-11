#include <algorithm>

#include "text_drawers_common.hpp"

namespace PanzerChasm
{

void CalculateLettersWidth(
	const unsigned char* const texture_data,
	unsigned char* const out_width )
{
	for( unsigned int code= 0u; code < 256u; code++ )
	{
		const unsigned int tc_u= ( code & 15u ) * FontParams::letter_place_width;
		const unsigned int tc_v= ( code >> 4u ) * FontParams::letter_place_height;

		unsigned int max_x= 0u;
		for( unsigned int y= tc_v + FontParams::letter_v_offset; y < tc_v + FontParams::letter_height; y++ )
		{
			unsigned int max_line_x= tc_u + FontParams::letter_u_offset;
			for( unsigned int x= tc_u + FontParams::letter_u_offset; x < tc_u + FontParams::letter_place_width; x++ )
			{
				if( texture_data[ x + y * FontParams::atlas_width  ] != 255 )
					max_line_x= std::max( max_line_x, x );
			}
			max_x= std::max( max_x, max_line_x );
		}

		out_width[code]= 1u + max_x - tc_u - FontParams::letter_u_offset;
	}

	out_width[' ']= FontParams::space_width;
}

} // namespace PanzerChasm
