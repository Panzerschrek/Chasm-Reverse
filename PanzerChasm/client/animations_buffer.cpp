#include <utility>

#include "animations_buffer.hpp"

namespace PanzerChasm
{

AnimationsBuffer AnimationsBuffer::AsTextureBuffer(
	const Model::AnimationVertex* const vertices,
	const unsigned int animation_vertex_count )
{
	AnimationsBuffer result;

	result.buffer_texture_=
		r_BufferTexture(
			r_Texture::PixelFormat::RGBA16I,
			animation_vertex_count * sizeof(Model::AnimationVertex),
			vertices );

	return result;
}

AnimationsBuffer AnimationsBuffer::As2dTexture(
	const Model::AnimationVertex* const vertices,
	const unsigned int animation_vertex_count )
{
	AnimationsBuffer result;

	const unsigned int height= ( animation_vertex_count + (c_2d_texture_width-1u) ) / c_2d_texture_width;

	result.texture_2d_=
		r_Texture(
			r_Texture::PixelFormat::RGBA16I,
			c_2d_texture_width,
			height,
			reinterpret_cast<const unsigned char*>(vertices) );

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
