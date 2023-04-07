#pragma once
#include "panzer_ogl_lib.hpp"

class r_PolygonBuffer
{
public:
	r_PolygonBuffer();
	r_PolygonBuffer( const r_PolygonBuffer& )= delete;
	r_PolygonBuffer( r_PolygonBuffer&& other );
	~r_PolygonBuffer();

	r_PolygonBuffer& operator=( const r_PolygonBuffer& )= delete;
	r_PolygonBuffer& operator=( r_PolygonBuffer&& other );

	void VertexData( const void* data, unsigned int d_size, unsigned int v_size );
	void VertexSubData( const void* data, unsigned int d_size, unsigned int shift );

	void IndexData( const void* data, unsigned int size, GLenum d_type, GLenum p_type );
	void IndexSubData( const void* data, unsigned int size, int shift );
	void VertexAttribPointer( int v_attrib, int components, GLenum type, bool normalize, int shift );
	void VertexAttribPointerInt( int v_attrib, int components, GLenum type, int stride );

	void Bind() const;

	void Draw() const;

	inline void SetPrimitiveType( GLenum t )
	{
		primitive_type_= t;
	}

private:
	unsigned int vertex_data_size_;
	unsigned int vertex_size_;

	GLenum index_data_type_;//type of index ( GL_UNSIGNED_INT / GL_UNSIGNED_SHORT )
	unsigned int index_data_size_;
	GLenum primitive_type_;

	GLuint v_buffer_, i_buffer_;
	GLuint v_array_object_;
};
