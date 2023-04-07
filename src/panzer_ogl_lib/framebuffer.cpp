#include <utility>

#include "panzer_ogl_lib.hpp"

#include "framebuffer.hpp"

const GLuint r_Framebuffer::c_screen_framebuffer_id_= 0;

unsigned int r_Framebuffer::screen_framebuffer_width_= 0, r_Framebuffer::screen_framebuffer_height_= 0;
unsigned int r_Framebuffer::current_framebuffer_width_= 0, r_Framebuffer::current_framebuffer_height_= 0;

GLuint r_Framebuffer::current_framebuffer_= r_Framebuffer::c_screen_framebuffer_id_;

unsigned int r_Framebuffer::CurrentFramebufferWidth ()
{
	return current_framebuffer_width_;
}

unsigned int r_Framebuffer::CurrentFramebufferHeight()
{
	return current_framebuffer_height_;
}

void r_Framebuffer::SetScreenFramebufferSize( unsigned int width, unsigned int height )
{
	screen_framebuffer_width_ = width ;
	screen_framebuffer_height_= height;
	if( current_framebuffer_ == c_screen_framebuffer_id_ )
	{
		current_framebuffer_width_ = width;
		current_framebuffer_height_= height;
	}
}

void r_Framebuffer::BindScreenFramebuffer()
{
	current_framebuffer_= c_screen_framebuffer_id_;
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	glViewport( 0, 0, screen_framebuffer_width_, screen_framebuffer_height_ );

	current_framebuffer_width_ = screen_framebuffer_width_ ;
	current_framebuffer_height_= screen_framebuffer_height_;
}

r_Framebuffer::r_Framebuffer()
	: framebuffer_id_(c_screen_framebuffer_id_)
	, size_x_(0), size_y_(0)
{
}

r_Framebuffer::r_Framebuffer( r_Framebuffer&& other )
{
	*this= std::move(other);
}

r_Framebuffer::r_Framebuffer(
	const std::vector< r_Texture::PixelFormat >& color_textures,
	r_Texture::PixelFormat depth_buffer_texture_format,
	unsigned int width, unsigned int height )
	: size_x_(width)
	, size_y_(height)
{
	glGenFramebuffers( 1, &framebuffer_id_ );
	glBindFramebuffer( GL_FRAMEBUFFER, framebuffer_id_ );

	textures_.resize( color_textures.size() );

	GLenum color_attachments[32];
	for( unsigned int i= 0; i< textures_.size(); i++ )
	{
		textures_[i]= r_Texture( color_textures[i], size_x_, size_y_ );
		textures_[i].SetFiltration( r_Texture::Filtration::Nearest, r_Texture::Filtration::Nearest );
		textures_[i].SetWrapMode( r_Texture::WrapMode::Clamp );

		glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, textures_[i].tex_id_, 0 );
		color_attachments[ i ]= GL_COLOR_ATTACHMENT0 + i;

	}// for textures
	glDrawBuffers( textures_.size(), color_attachments );

	if( depth_buffer_texture_format != r_Texture::PixelFormat::Unknown )
	{
		depth_texture_= r_Texture( depth_buffer_texture_format, size_x_, size_y_ );
		depth_texture_.SetFiltration( r_Texture::Filtration::Nearest, r_Texture::Filtration::Nearest );
		depth_texture_.SetWrapMode( r_Texture::WrapMode::Clamp );

		GLenum attach_type=
			depth_buffer_texture_format == r_Texture::PixelFormat::Depth24Stencil8
				? GL_DEPTH_STENCIL_ATTACHMENT
				: GL_DEPTH_ATTACHMENT;
		glFramebufferTexture( GL_FRAMEBUFFER, attach_type, depth_texture_.tex_id_, 0 );
	}

	glBindFramebuffer( GL_FRAMEBUFFER, current_framebuffer_ );
}

r_Framebuffer::~r_Framebuffer()
{
	DeleteFramebufferObject();
}

r_Framebuffer& r_Framebuffer::operator=( r_Framebuffer&& other )
{
	DeleteFramebufferObject();

	framebuffer_id_= other.framebuffer_id_;
	other.framebuffer_id_= c_screen_framebuffer_id_;

	textures_= std::move(other.textures_);
	other.textures_.clear();

	depth_texture_= std::move(other.depth_texture_);

	size_x_= other.size_x_;
	other.size_x_= 0;
	size_y_= other.size_y_;
	other.size_y_= 0;

	return *this;
}

void r_Framebuffer::Bind()
{
	current_framebuffer_= framebuffer_id_;
	glBindFramebuffer( GL_FRAMEBUFFER, framebuffer_id_ );
	glViewport( 0, 0, size_x_, size_y_ );

	current_framebuffer_width_ = size_x_;
	current_framebuffer_height_= size_y_;
}

bool r_Framebuffer::IsEmpty() const
{
	return framebuffer_id_ == c_screen_framebuffer_id_;
}

unsigned int r_Framebuffer::Width () const
{
	return size_x_;
}

unsigned int r_Framebuffer::Height() const
{
	return size_y_;
}

const std::vector<r_Texture>& r_Framebuffer::GetTextures() const
{
	return textures_;
}

std::vector<r_Texture>& r_Framebuffer::GetTextures()
{
	return textures_;
}

const r_Texture& r_Framebuffer::GetDepthTexture() const
{
	return depth_texture_;
}

r_Texture& r_Framebuffer::GetDepthTexture()
{
	return depth_texture_;
}

void r_Framebuffer::DeleteFramebufferObject()
{
	if( framebuffer_id_ != c_screen_framebuffer_id_ )
	{
		if( framebuffer_id_ == current_framebuffer_ )
		{
			current_framebuffer_= c_screen_framebuffer_id_;

			current_framebuffer_width_ = screen_framebuffer_width_ ;
			current_framebuffer_height_= screen_framebuffer_height_;

			glBindFramebuffer( GL_FRAMEBUFFER, c_screen_framebuffer_id_ );
		}
		glDeleteFramebuffers( 1, &framebuffer_id_ );
	}
}
