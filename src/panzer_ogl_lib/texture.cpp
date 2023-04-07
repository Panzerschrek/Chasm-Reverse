#include <limits>
#include <utility>

#include "panzer_ogl_lib.hpp"

#include "texture.hpp"

const GLuint r_Texture::c_texture_not_created_= std::numeric_limits<GLuint>::max();

GLenum r_Texture::FormatToInternalFormat( PixelFormat format )
{
	switch(format)
	{
		case PixelFormat::Unknown: return 0;

		// 1 component
		case PixelFormat::R8:
			return GL_R8;
		case PixelFormat::R8S:
			return GL_R8_SNORM;
		case PixelFormat::R8UI:
			return GL_R8UI;
		case PixelFormat::R8I:
			return GL_R8I;

		case PixelFormat::R16:
			return GL_R16;
		case PixelFormat::R16S:
			return GL_R16_SNORM;
		case PixelFormat::R16UI:
			return GL_R16UI;
		case PixelFormat::R16I:
			return GL_R16I;
		case PixelFormat::R16F:
			return GL_R16F;

		case PixelFormat::R32UI:
			return GL_R32UI;
		case PixelFormat::R32I:
			return GL_R32I;
		case PixelFormat::R32F:
			return GL_R32F;

		// 2 component
		case PixelFormat::RG8:
			return GL_RG8;
		case PixelFormat::RG8S:
			return GL_RG8_SNORM;
		case PixelFormat::RG8UI:
			return GL_RG8UI;
		case PixelFormat::RG8I:
			return GL_RG8I;

		case PixelFormat::RG16:
			return GL_RG16;
		case PixelFormat::RG16S:
			return GL_RG16_SNORM;
		case PixelFormat::RG16UI:
			return GL_RG16UI;
		case PixelFormat::RG16I:
			return GL_RG16I;
		case PixelFormat::RG16F:
			return GL_RG16F;

		case PixelFormat::RG32UI:
			return GL_RG32UI;
		case PixelFormat::RG32I:
			return GL_RG32I;
		case PixelFormat::RG32F:
			return GL_RG32F;

		// 4 component
		case PixelFormat::RGBA8:
			return GL_RGBA8;
		case PixelFormat::RGBA8S:
			return GL_RGBA8_SNORM;
		case PixelFormat::RGBA8UI:
			return GL_RGBA8UI;
		case PixelFormat::RGBA8I:
			return GL_RGBA8I;

		case PixelFormat::RGBA16:
			return GL_RGBA16;
		case PixelFormat::RGBA16S:
			return GL_RGBA16_SNORM;
		case PixelFormat::RGBA16UI:
			return GL_RGBA16UI;
		case PixelFormat::RGBA16I:
			return GL_RGBA16I;
		case PixelFormat::RGBA16F:
			return GL_RGBA16F;

		case PixelFormat::RGBA32UI:
			return GL_RGBA32UI;
		case PixelFormat::RGBA32I:
			return GL_RGBA32I;
		case PixelFormat::RGBA32F:
			return GL_RGBA32F;

		// depth
		case PixelFormat::Depth16:
			return GL_DEPTH_COMPONENT16;
		case PixelFormat::Depth32:
			return GL_DEPTH_COMPONENT32;
		case PixelFormat::Depth32F:
			return GL_DEPTH_COMPONENT32F;
		case PixelFormat::Depth24Stencil8:
			return GL_DEPTH24_STENCIL8;
	};

	return 0;
}

GLenum r_Texture::FormatToBaseFormat( PixelFormat format )
{
	switch(format)
	{
		case PixelFormat::Unknown: return 0;

		// 1 component
		case PixelFormat::R8:
		case PixelFormat::R8S:

		case PixelFormat::R16:
		case PixelFormat::R16S:

		case PixelFormat::R16F:

		case PixelFormat::R32F:
			return GL_RED;

		case PixelFormat::R8UI:
		case PixelFormat::R8I:
		case PixelFormat::R16UI:
		case PixelFormat::R16I:
		case PixelFormat::R32UI:
		case PixelFormat::R32I:
			return GL_RED_INTEGER;

		// 2 component
		case PixelFormat::RG8:
		case PixelFormat::RG8S:

		case PixelFormat::RG16:
		case PixelFormat::RG16S:

		case PixelFormat::RG16F:

		case PixelFormat::RG32F:
			return GL_RG;

		case PixelFormat::RG8UI:
		case PixelFormat::RG8I:
		case PixelFormat::RG16UI:
		case PixelFormat::RG16I:
		case PixelFormat::RG32UI:
		case PixelFormat::RG32I:
			return GL_RG_INTEGER;

		// 4 component
		case PixelFormat::RGBA8:
		case PixelFormat::RGBA8S:

		case PixelFormat::RGBA16:
		case PixelFormat::RGBA16S:

		case PixelFormat::RGBA16F:

		case PixelFormat::RGBA32F:
			return GL_RGBA;

		case PixelFormat::RGBA8UI:
		case PixelFormat::RGBA8I:
		case PixelFormat::RGBA16UI:
		case PixelFormat::RGBA16I:
		case PixelFormat::RGBA32UI:
		case PixelFormat::RGBA32I:
			return GL_RGBA_INTEGER;

		// depth
		case PixelFormat::Depth16:
		case PixelFormat::Depth32:
		case PixelFormat::Depth32F:
			return GL_DEPTH_COMPONENT;
		case PixelFormat::Depth24Stencil8:
			return GL_DEPTH_STENCIL;
	};

	return 0;
}

GLenum r_Texture::FormatToDataType( PixelFormat format )
{
	switch(format)
	{
		case PixelFormat::Unknown: return 0;

		// 1 component
		case PixelFormat::R8:
			return GL_UNSIGNED_BYTE;
		case PixelFormat::R8S:
			return GL_BYTE;
		case PixelFormat::R8UI:
			return GL_UNSIGNED_BYTE;
		case PixelFormat::R8I:
			return GL_BYTE;

		case PixelFormat::R16:
			return GL_UNSIGNED_SHORT;
		case PixelFormat::R16S:
			return GL_SHORT;
		case PixelFormat::R16UI:
			return GL_UNSIGNED_SHORT;
		case PixelFormat::R16I:
			return GL_SHORT;
		case PixelFormat::R16F:
			return GL_FLOAT;

		case PixelFormat::R32UI:
			return GL_UNSIGNED_INT;
		case PixelFormat::R32I:
			return GL_INT;
		case PixelFormat::R32F:
			return GL_FLOAT;

		// 2 component
		case PixelFormat::RG8:
			return GL_UNSIGNED_BYTE;
		case PixelFormat::RG8S:
			return GL_BYTE;
		case PixelFormat::RG8UI:
			return GL_UNSIGNED_BYTE;
		case PixelFormat::RG8I:
			return GL_BYTE;

		case PixelFormat::RG16:
			return GL_UNSIGNED_SHORT;
		case PixelFormat::RG16S:
			return GL_SHORT;
		case PixelFormat::RG16UI:
			return GL_UNSIGNED_SHORT;
		case PixelFormat::RG16I:
			return GL_SHORT;
		case PixelFormat::RG16F:
			return GL_FLOAT;

		case PixelFormat::RG32UI:
			return GL_UNSIGNED_INT;
		case PixelFormat::RG32I:
			return GL_INT;
		case PixelFormat::RG32F:
			return GL_FLOAT;

		// 4 component
		case PixelFormat::RGBA8:
			return GL_UNSIGNED_BYTE;
		case PixelFormat::RGBA8S:
			return GL_BYTE;
		case PixelFormat::RGBA8UI:
			return GL_UNSIGNED_BYTE;
		case PixelFormat::RGBA8I:
			return GL_BYTE;

		case PixelFormat::RGBA16:
			return GL_UNSIGNED_SHORT;
		case PixelFormat::RGBA16S:
			return GL_SHORT;
		case PixelFormat::RGBA16UI:
			return GL_UNSIGNED_SHORT;
		case PixelFormat::RGBA16I:
			return GL_SHORT;
		case PixelFormat::RGBA16F:
			return GL_FLOAT;

		case PixelFormat::RGBA32UI:
			return GL_UNSIGNED_INT;
		case PixelFormat::RGBA32I:
			return GL_INT;
		case PixelFormat::RGBA32F:
			return GL_FLOAT;

		// depth
		case PixelFormat::Depth16:
			return GL_UNSIGNED_SHORT;
		case PixelFormat::Depth32:
			return GL_UNSIGNED_INT;
		case PixelFormat::Depth32F:
			return GL_FLOAT;
		case PixelFormat::Depth24Stencil8:
			return GL_UNSIGNED_INT_24_8;
	};

	return 0;
}

GLenum r_Texture::FiltrationToGLFiltration( Filtration filtration )
{
	switch(filtration)
	{
		case Filtration::Unknown: return 0;

		case Filtration::Nearest:
			return GL_NEAREST;
		case Filtration::Linear:
			return GL_LINEAR;
		case Filtration::NearestMipmapNearest:
			return GL_NEAREST_MIPMAP_NEAREST;
		case Filtration::NearestMipmapLinear:
			return GL_NEAREST_MIPMAP_LINEAR;
		case Filtration::LinearMipmapNearest:
			return GL_LINEAR_MIPMAP_NEAREST;
		case Filtration::LinearMipmapLinear:
			return GL_LINEAR_MIPMAP_LINEAR;
	};

	return 0;
}

GLenum r_Texture::WrapModeToGLWrapMode( WrapMode mode )
{
	switch(mode)
	{
		case WrapMode::Unknown: return 0;

		case WrapMode::Clamp:
			return GL_CLAMP_TO_EDGE;
		case WrapMode::Repeat:
			return GL_REPEAT;
	};

	return 0;
}

GLenum r_Texture::CompareModeToGLCompareFunc( CompareMode mode )
{
	switch(mode)
	{
		case CompareMode::Unknown:
		case CompareMode::None:
			return 0;

		case CompareMode::Less:
			return GL_LESS;
		case CompareMode::Greater:
			return GL_GREATER;
		case CompareMode::Equal:
			return GL_EQUAL;
	};

	return 0;
}

r_Texture::r_Texture()
	: tex_id_( c_texture_not_created_ )
	, format_( PixelFormat::Unknown )
	, wrap_mode_( WrapMode::Repeat )
	, compare_mode_( CompareMode::None )
	, filter_min_( Filtration::NearestMipmapLinear )
	, filter_mag_( Filtration::Linear )
	, size_x_(0), size_y_(0)
{
}

r_Texture::r_Texture( r_Texture&& other )
{
	*this= std::move(other);
}

r_Texture::r_Texture( PixelFormat f, unsigned int width, unsigned int height )
	: tex_id_( c_texture_not_created_ )
	, format_( f )
	, wrap_mode_( WrapMode::Repeat )
	, compare_mode_( CompareMode::None )
	, filter_min_( Filtration::NearestMipmapLinear )
	, filter_mag_( Filtration::Linear )
	, size_x_(width), size_y_(height)
{
	glGenTextures( 1, &tex_id_ );
	glBindTexture( GL_TEXTURE_2D, tex_id_ );

	SetData( nullptr );
}

r_Texture::r_Texture( PixelFormat f, unsigned int width, unsigned int height, const unsigned char* data )
	: tex_id_( c_texture_not_created_ )
	, format_( f )
	, wrap_mode_( WrapMode::Repeat )
	, compare_mode_( CompareMode::None )
	, filter_min_( Filtration::NearestMipmapLinear )
	, filter_mag_( Filtration::Linear )
	, size_x_(width), size_y_(height)
{
	glGenTextures( 1, &tex_id_ );
	glBindTexture( GL_TEXTURE_2D, tex_id_ );

	SetData( data );
}

r_Texture::r_Texture( PixelFormat f, unsigned int width, unsigned int height, const float* data )
	: tex_id_( c_texture_not_created_ )
	, format_( f )
	, wrap_mode_( WrapMode::Repeat )
	, compare_mode_( CompareMode::None )
	, filter_min_( Filtration::NearestMipmapLinear )
	, filter_mag_( Filtration::Linear )
	, size_x_(width), size_y_(height)
{
	glGenTextures( 1, &tex_id_ );
	glBindTexture( GL_TEXTURE_2D, tex_id_ );

	SetData( data );
}

r_Texture::~r_Texture()
{
	if( tex_id_ != c_texture_not_created_ )
		glDeleteTextures( 1, &tex_id_ );
}

r_Texture& r_Texture::operator=( r_Texture&& other )
{
	if( tex_id_ != c_texture_not_created_ )
		glDeleteTextures( 1, &tex_id_ );

	tex_id_= other.tex_id_;
	other.tex_id_= c_texture_not_created_;

	format_= other.format_;
	other.format_= PixelFormat::Unknown;

	wrap_mode_= other.wrap_mode_;
	other.wrap_mode_= WrapMode::Repeat;

	compare_mode_= other.compare_mode_;
	other.compare_mode_= CompareMode::None;

	filter_min_= other.filter_min_;
	other.filter_min_= Filtration::NearestMipmapLinear;

	filter_mag_= other.filter_mag_;
	other.filter_mag_= Filtration::Linear;

	size_x_= other.size_x_;
	other.size_x_= 0;
	size_y_= other.size_y_;
	other.size_y_= 0;

	return *this;
}

void r_Texture::SetData( const void* data )
{
	GLenum gl_format= FormatToInternalFormat( format_ );
	GLenum component_format= FormatToBaseFormat( format_ );
	GLenum data_format= FormatToDataType( format_ );

	glTexImage2D(
		GL_TEXTURE_2D, 0, gl_format,
		size_x_, size_y_, 0,
		component_format, data_format, data );
}

void r_Texture::SetWrapMode( WrapMode mode )
{
	if( wrap_mode_ != mode )
	{
		wrap_mode_= mode;
		GLenum gl_wrap_mode= WrapModeToGLWrapMode(wrap_mode_);
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gl_wrap_mode );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gl_wrap_mode );
	}
}

void r_Texture::SetFiltration( Filtration f_min, Filtration f_mag )
{
	if( f_min != filter_min_ )
	{
		filter_min_= f_min;
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, FiltrationToGLFiltration(filter_min_) );
	}
	if( f_mag != filter_mag_ )
	{
		filter_mag_= f_mag;
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, FiltrationToGLFiltration(filter_mag_) );
	}
}

void r_Texture::SetCompareMode( CompareMode mode )
{
	if( mode != compare_mode_ )
	{
		compare_mode_= mode;

		if( compare_mode_ != CompareMode::None )
		{
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, CompareModeToGLCompareFunc(compare_mode_) );
		}
		else
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE );
	}
}

void r_Texture::BuildMips()
{
	glGenerateMipmap( GL_TEXTURE_2D );
}

void r_Texture::Bind( unsigned int unit ) const
{
	glActiveTexture( GL_TEXTURE0 + unit );
	glBindTexture( GL_TEXTURE_2D, tex_id_ );
}

bool r_Texture::IsEmpty() const
{
	return tex_id_ == c_texture_not_created_;
}

unsigned int r_Texture::Width () const
{
	return size_x_;
}

unsigned int r_Texture::Height() const
{
	return size_y_;
}

bool r_Texture::IsDepthTexture() const
{
	return
		format_ == PixelFormat::Depth16  ||
		format_ == PixelFormat::Depth32  ||
		format_ == PixelFormat::Depth32F ||
		format_ == PixelFormat::Depth24Stencil8;
}

