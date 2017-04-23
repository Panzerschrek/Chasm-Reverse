#include <utility>

#include "animations_buffer.hpp"

namespace PanzerChasm
{

AnimationsBuffer AnimationsBuffer::AsTextureBuffer( Model::AnimationsVertices& vertices )
{
	AnimationsBuffer result;

	result.buffer_texture_=
		r_BufferTexture(
			r_Texture::PixelFormat::RGBA16I,
			vertices.size() * sizeof(Model::AnimationVertex),
			vertices.data() );

	return result;
}

AnimationsBuffer AnimationsBuffer::As2dTexture( Model::AnimationsVertices& vertices )
{
	AnimationsBuffer result;

	const unsigned int height= ( vertices.size() + (c_2d_texture_width-1u) ) / c_2d_texture_width;

	// Resize vector. We must have valid memory block of size width * height.
	const Model::AnimationVertex zero_vertex{ 0, 0, 0, 0 };
	vertices.resize( c_2d_texture_width * height, zero_vertex );

	result.texture_2d_=
		r_Texture(
			r_Texture::PixelFormat::RGBA16I,
			c_2d_texture_width,
			height,
			reinterpret_cast<const unsigned char*>(vertices.data()) );

	result.texture_2d_.SetFiltration( r_Texture::Filtration::Nearest, r_Texture::Filtration::Nearest );

	return result;
}

AnimationsBuffer::AnimationsBuffer()
{}

AnimationsBuffer::AnimationsBuffer( AnimationsBuffer&& other )
{
	*this= std::move(other);
}

AnimationsBuffer::~AnimationsBuffer()
{}

AnimationsBuffer& AnimationsBuffer::operator=( AnimationsBuffer&& other )
{
	texture_2d_= std::move( other.texture_2d_ );
	buffer_texture_= std::move( other.buffer_texture_ );

	return *this;
}

void AnimationsBuffer::Bind( const unsigned int unit )
{
	if( !texture_2d_.IsEmpty() )
		texture_2d_.Bind( unit );
	else
		buffer_texture_.Bind( unit );
}

} // PanzerChasm
