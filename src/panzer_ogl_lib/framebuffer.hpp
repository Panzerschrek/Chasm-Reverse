#pragma once

#include <vector>

#include "panzer_ogl_lib.hpp"
#include "texture.hpp"

class r_Framebuffer final
{
public:
	static unsigned int CurrentFramebufferWidth ();
	static unsigned int CurrentFramebufferHeight();
	static void SetScreenFramebufferSize( unsigned int width, unsigned int height );
	static void BindScreenFramebuffer();

	r_Framebuffer();
	r_Framebuffer( const r_Framebuffer& )= delete;
	r_Framebuffer( r_Framebuffer&& other );
	r_Framebuffer(
		const std::vector< r_Texture::PixelFormat >& color_textures,
		r_Texture::PixelFormat depth_buffer_texture_format,
		unsigned int width, unsigned int height );
	~r_Framebuffer();

	r_Framebuffer& operator=( const r_Framebuffer& )= delete;
	r_Framebuffer& operator=( r_Framebuffer&& other );

	void Bind();

	bool IsEmpty() const;
	unsigned int Width () const;
	unsigned int Height() const;

	const std::vector< r_Texture >& GetTextures() const;
	std::vector< r_Texture >& GetTextures();
	const r_Texture& GetDepthTexture() const;
	r_Texture& GetDepthTexture();

private:
	void DeleteFramebufferObject();

private:
	GLuint framebuffer_id_;
	unsigned int size_x_, size_y_;

	std::vector<r_Texture> textures_;
	r_Texture depth_texture_;

	static const GLuint c_screen_framebuffer_id_;
	static unsigned int screen_framebuffer_width_ , screen_framebuffer_height_ ;
	static unsigned int current_framebuffer_width_, current_framebuffer_height_;
	static GLuint current_framebuffer_;
};
