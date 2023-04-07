#pragma once

#include <string>
#include <vector>

#include "panzer_ogl_lib.hpp"

#include "vec.hpp"
#include "matrix.hpp"

class r_GLSLProgram final
{
public:
	typedef void (*ProgramBuildLogOutCallback)( const char* text );
	static void SetProgramBuildLogOutCallback( ProgramBuildLogOutCallback callback );

	r_GLSLProgram();
	r_GLSLProgram( const r_GLSLProgram& )= delete;
	~r_GLSLProgram();

	r_GLSLProgram& operator=( r_GLSLProgram& )= delete;

	void ShaderSource( std::string frag_text, std::string vert_tezt, std::string geom_text= std::string() );

	// Call before shader compilation.
	void SetAttribLocation( const char* name, unsigned int location );

	void Create(); // compiles shader and link program object
	void Destroy(); // deletes shaders and program object

	void Bind() const;

	// int
	void Uniform( const char* name, int i );
	// int array
	void Uniform( const char* name, int* i, unsigned int count );
	// float
	void Uniform( const char* name, float f );
	// float array
	void Uniform( const char* name, const float* f, unsigned int count );
	// vec2
	void Uniform( const char* name, const m_Vec2& v );
	// vec2 array
	void Uniform( const char* name, const m_Vec2* v, unsigned int count );
	// mat3
	// vec3
	void Uniform( const char* name, const m_Vec3& v );
	// vec3 array
	void Uniform( const char* name, const m_Vec3* v, unsigned int count );
	// mat3
	void Uniform( const char* name, const m_Mat3& m );
	// mat3 array
	void Uniform( const char* name, const m_Mat3* m, unsigned int count );
	// mat4
	void Uniform( const char* name, const m_Mat4& m );
	// mat4 array
	void Uniform( const char* name, const m_Mat4* m, unsigned int count );
	// vec4
	void Uniform( const char* name, float f0, float f1, float f2, float f3 );

private:
	struct Uniform_s
	{
		std::string name;
		GLint id;
	};

	struct Attrib_s
	{
		std::string name;
		GLint location;
	};

private:
	void FindUniforms();
	int GetUniform( const char* name );

private:
	GLuint prog_handle_;
	GLuint frag_handle_, vert_handle_, geom_handle_;
	std::string vert_text_;
	std::string frag_text_;
	std::string geom_text_;

	std::vector<Uniform_s> uniforms_;
	std::vector<Attrib_s> attribs_;

	static ProgramBuildLogOutCallback build_log_out_callback_;
};
