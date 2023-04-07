#include <limits>
#include <utility>

#include "polygon_buffer.hpp"

static const GLuint c_buffer_not_created= std::numeric_limits<GLuint>::max();

r_PolygonBuffer::r_PolygonBuffer()
	: vertex_data_size_(0), vertex_size_(0)
	, index_data_type_(GL_NONE), index_data_size_(0)
	, primitive_type_(GL_NONE)
	, v_buffer_(c_buffer_not_created), i_buffer_(c_buffer_not_created)
	, v_array_object_(c_buffer_not_created)
{
}

r_PolygonBuffer::r_PolygonBuffer( r_PolygonBuffer&& other )
{
	*this= std::move(other);
}

r_PolygonBuffer::~r_PolygonBuffer()
{
	if( v_buffer_!= c_buffer_not_created )
		glDeleteBuffers( 1, &v_buffer_ );

	if( i_buffer_!= c_buffer_not_created )
		glDeleteBuffers( 1, &i_buffer_ );

	if( v_array_object_ != c_buffer_not_created )
		glDeleteVertexArrays( 1, &v_array_object_ );
}

r_PolygonBuffer& r_PolygonBuffer::operator=( r_PolygonBuffer&& other )
{
	if( v_buffer_!= c_buffer_not_created )
		glDeleteBuffers( 1, &v_buffer_ );

	if( i_buffer_!= c_buffer_not_created )
		glDeleteBuffers( 1, &i_buffer_ );

	if( v_array_object_ != c_buffer_not_created )
		glDeleteVertexArrays( 1, &v_array_object_ );

	v_buffer_= other.v_buffer_;
	other.v_buffer_= c_buffer_not_created;

	i_buffer_= other.i_buffer_;
	other.i_buffer_= c_buffer_not_created;

	v_array_object_= other.v_array_object_;
	other.v_array_object_= c_buffer_not_created;

	vertex_data_size_= other.vertex_data_size_;
	other.vertex_data_size_= 0;

	vertex_size_= other.vertex_size_;
	other.vertex_size_= 0;

	index_data_type_= other.index_data_type_;
	other.index_data_type_= GL_NONE;

	index_data_size_= other.index_data_size_;
	other.index_data_size_= 0;

	primitive_type_= other.primitive_type_;
	other.primitive_type_= GL_NONE;

	return *this;
}

void r_PolygonBuffer::VertexData( const void* data, unsigned int d_size, unsigned int v_size )
{
	if( v_array_object_ == c_buffer_not_created )
		glGenVertexArrays( 1, &v_array_object_ );
	glBindVertexArray( v_array_object_ );

	if( v_buffer_ == c_buffer_not_created )
		glGenBuffers( 1, &v_buffer_ );

	glBindBuffer( GL_ARRAY_BUFFER, v_buffer_ );
	glBufferData( GL_ARRAY_BUFFER, d_size, data, GL_STATIC_DRAW );

	vertex_data_size_= d_size;
	vertex_size_= v_size;
}

void r_PolygonBuffer::VertexSubData( const void* data, unsigned int d_size, unsigned int shift )
{
	if( v_buffer_ == c_buffer_not_created || v_array_object_ == c_buffer_not_created )
		return;
	glBindVertexArray( v_array_object_ );

	glBindBuffer( GL_ARRAY_BUFFER, v_buffer_ );
	glBufferSubData( GL_ARRAY_BUFFER, shift, d_size, data );
}

void r_PolygonBuffer::IndexData( const void* data, unsigned int size, GLenum d_type, GLenum p_type )
{
	if( v_array_object_ == c_buffer_not_created )
		glGenVertexArrays( 1, &v_array_object_ );
	glBindVertexArray( v_array_object_ );

	if( i_buffer_ == c_buffer_not_created )
		glGenBuffers( 1, &i_buffer_ );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, i_buffer_ );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, size, data, GL_STATIC_DRAW );

	index_data_size_= size;
	index_data_type_= d_type;
	primitive_type_= p_type;
}

void r_PolygonBuffer::IndexSubData( const void* data, unsigned int size, int shift )
{
	if( i_buffer_ == c_buffer_not_created || v_array_object_ == c_buffer_not_created  )
		return;

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, i_buffer_ );
	glBufferSubData( GL_ELEMENT_ARRAY_BUFFER, shift, size, data );
}

void r_PolygonBuffer::Bind() const
{
	if( v_array_object_ != c_buffer_not_created )
		glBindVertexArray( v_array_object_ );
}

void r_PolygonBuffer::Draw() const
{
	Bind();

	int s;
	if( i_buffer_ == c_buffer_not_created )
	{
		if( v_buffer_ == c_buffer_not_created )
			return;
		s= vertex_data_size_ / vertex_size_;
		glDrawArrays( primitive_type_, 0, s );
	}
	else
	{
		if( v_buffer_ == c_buffer_not_created )
			return;
		s= index_data_type_ == GL_UNSIGNED_INT ? 4 : 2;
		s= index_data_size_ / s;
		glDrawElements( primitive_type_, s, index_data_type_, NULL );
	}
}

void r_PolygonBuffer::VertexAttribPointer( int v_attrib, int components, GLenum type, bool normalize, int shift )
{
	if( v_array_object_ == c_buffer_not_created || v_buffer_ == c_buffer_not_created )
		return;

	glBindVertexArray( v_array_object_ );
	glBindBuffer( GL_ARRAY_BUFFER, v_buffer_ );

	glEnableVertexAttribArray( v_attrib );
	glVertexAttribPointer( v_attrib, components, type, normalize, vertex_size_, (void*) shift );
}

void r_PolygonBuffer::VertexAttribPointerInt( int v_attrib, int components, GLenum type, int shift )
{
	if( v_array_object_ == c_buffer_not_created || v_buffer_ == c_buffer_not_created )
		return;

	glBindVertexArray( v_array_object_ );
	glBindBuffer( GL_ARRAY_BUFFER, v_buffer_ );

	glEnableVertexAttribArray( v_attrib );
	glVertexAttribIPointer( v_attrib, components, type, vertex_size_, (void*) shift );
}
