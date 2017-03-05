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
			static_cast<const unsigned char*>(nullptr) );

	glTexSubImage2D(
		GL_TEXTURE_2D, 0,
		0, 0,
		c_2d_texture_width, animation_vertex_count / c_2d_texture_width,
		GL_RGBA_INTEGER,
		GL_SHORT,
		vertices );

	if( ( animation_vertex_count % c_2d_texture_width ) != 0u )
	{
		unsigned int last_line_vertex_count= animation_vertex_count - ( height - 1u ) * c_2d_texture_width;

		glTexSubImage2D(
			GL_TEXTURE_2D, 0,
			0, height - 1u,
			last_line_vertex_count, 1u,
			GL_RGBA_INTEGER,
			GL_SHORT,
			vertices + animation_vertex_count - last_line_vertex_count );
	}

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
