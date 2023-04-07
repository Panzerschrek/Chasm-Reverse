#include <limits>
#include <utility>

#include "buffer_texture.hpp"

const GLuint r_BufferTexture::c_texture_not_created_= std::numeric_limits<GLuint>::max();

r_BufferTexture::r_BufferTexture()
{}

r_BufferTexture::r_BufferTexture( r_BufferTexture&& other )
{
	*this= std::move(other);
}

r_BufferTexture::r_BufferTexture( const PixelFormat format, const unsigned int data_size, const void* const data )
{
	glGenBuffers( 1, &buffer_id_ );
	glBindBuffer( GL_TEXTURE_BUFFER, buffer_id_ );
	glBufferData( GL_TEXTURE_BUFFER, data_size, data, GL_STATIC_DRAW );

	glGenTextures( 1, &texture_id_ );
	glBindTexture( GL_TEXTURE_BUFFER, texture_id_ );
	//glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // did this really need?
	glTexBuffer( GL_TEXTURE_BUFFER, r_Texture::FormatToInternalFormat( format ), buffer_id_ );
}

r_BufferTexture::~r_BufferTexture()
{
	if( texture_id_ != c_texture_not_created_ )
		glDeleteTextures( 1, &texture_id_ );
	if( buffer_id_ != c_texture_not_created_ )
		glDeleteBuffers( 1, &buffer_id_ );
}

r_BufferTexture& r_BufferTexture::operator=( r_BufferTexture&& other )
{
	if( texture_id_ != c_texture_not_created_ )
		glDeleteTextures( 1, &texture_id_ );
	if( buffer_id_ != c_texture_not_created_ )
		glDeleteBuffers( 1, &buffer_id_ );

	texture_id_= other.texture_id_;
	other.texture_id_= c_texture_not_created_;

	buffer_id_= other.buffer_id_;
	other.buffer_id_= c_texture_not_created_;

	return *this;
}

void r_BufferTexture::Bind( const unsigned int unit ) const
{
	glActiveTexture( GL_TEXTURE0 + unit );
	glBindTexture( GL_TEXTURE_BUFFER, texture_id_ );
}

bool r_BufferTexture::IsEmpty() const
{
	return texture_id_ == c_texture_not_created_;
}
