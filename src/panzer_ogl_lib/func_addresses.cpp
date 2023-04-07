#include "panzer_ogl_lib.hpp"

#define PROCESS_OGL_FUNCTION( TYPE, NAME ) TYPE NAME= nullptr
#include "functions_list.h"
#undef PROCESS_OGL_FUNCTION

void GetGLFunctions(
	void* (*GetProcAddressFunc)(const char* func_name),
	void (*FuncNotFoundCallback)(const char* func_name) )
{
	#define PROCESS_OGL_FUNCTION( TYPE, NAME )\
	NAME= (TYPE) GetProcAddressFunc( #NAME );\
	if( !NAME && FuncNotFoundCallback )\
		FuncNotFoundCallback( #NAME );

	#include "functions_list.h"

	#undef PROCESS_OGL_FUNCTION
}
