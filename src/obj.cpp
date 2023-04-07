#include <algorithm>
#include <cstring>

#include "assert.hpp"

#include "obj.hpp"

namespace PanzerChasm
{

struct FrameHeader
{
	unsigned short size[2];
	unsigned short x_center;
};

SIZE_ASSERT( FrameHeader, 6 );

void LoadObjSprite( const Vfs::FileContent& obj_file, ObjSprite& out_sprite )
{
	unsigned short frame_count;
	std::memcpy( &frame_count, obj_file.data(), sizeof(frame_count) );

	unsigned int max_size[2]= { 0u, 0u };

	// Calculate max size
	{
		const unsigned char* ptr= obj_file.data() + sizeof(frame_count);
		for( unsigned int f= 0u; f < frame_count; f++ )
		{
			const FrameHeader* const header= reinterpret_cast<const FrameHeader*>(ptr);
			max_size[0]= std::max( max_size[0], std::max( (unsigned int) header->size[0], (unsigned int) header->x_center * 2u ) );
			max_size[1]= std::max( max_size[1], (unsigned int) header->size[1] );
			ptr+= sizeof(FrameHeader) + header->size[0] * header->size[1];
		}
	}

	out_sprite.size[0]= max_size[0];
	out_sprite.size[1]= max_size[1];
	out_sprite.frame_count= frame_count;
	out_sprite.data.resize( out_sprite.size[0] * out_sprite.size[1] * out_sprite.frame_count, 255u );

	const unsigned char* ptr= obj_file.data() + sizeof(frame_count);
	for( unsigned int f= 0u; f < frame_count; f++ )
	{
		const FrameHeader* const header= reinterpret_cast<const FrameHeader*>(ptr);

		// Shift this frame to center
		const unsigned int x_offset= out_sprite.size[0] / 2u - header->x_center;
		const unsigned int y_offset= 0u;
		PC_ASSERT( out_sprite.size[0] / 2u >= header->x_center );
		PC_ASSERT( x_offset + header->size[0] <= out_sprite.size[0] );
		PC_ASSERT( y_offset + header->size[1] <= out_sprite.size[1] );

		const unsigned char* const src= ptr + sizeof(FrameHeader);
		unsigned char* const dst= out_sprite.data.data() + out_sprite.size[0] * out_sprite.size[1] * f;

		// Copy and flip
		for( unsigned int y= 0u; y < header->size[1]; y++ )
		for( unsigned int x= 0u; x < header->size[0]; x++ )
			dst[ x_offset + x + ( y + y_offset ) * out_sprite.size[0] ]=
				src[ ( header->size[1] - 1u - y ) + x * header->size[1] ];

		ptr+= sizeof(FrameHeader) + header->size[0] * header->size[1];
	}
}

} // namespace PanzerChasm
