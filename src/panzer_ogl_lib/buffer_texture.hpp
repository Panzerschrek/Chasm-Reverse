#pragma once

#include "panzer_ogl_lib.hpp"
#include "texture.hpp"

class r_BufferTexture final
{
public:
	typedef r_Texture::PixelFormat PixelFormat;

	r_BufferTexture();
	r_BufferTexture( const r_BufferTexture& )= delete;
	r_BufferTexture( r_BufferTexture&& other );

	//create and bind texture buffer
	r_BufferTexture( PixelFormat format, unsigned int data_size, const void* data );

	~r_BufferTexture();

	r_BufferTexture& operator=( const r_BufferTexture& )= delete;
	r_BufferTexture& operator=( r_BufferTexture&& other );

	//bind texture and set active texture unit.
	void Bind( unsigned int unit= 0 ) const;

	bool IsEmpty() const;

private:
	static const GLuint c_texture_not_created_;

private:
	GLuint texture_id_= c_texture_not_created_;
	GLuint buffer_id_= c_texture_not_created_;
};
