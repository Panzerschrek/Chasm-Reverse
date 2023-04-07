#pragma once

#define PROCESS_OGL_FUNCTION( TYPE, NAME ) extern TYPE NAME
#include "functions_list.h"
#undef PROCESS_OGL_FUNCTION

void GetGLFunctions(
	void* (*GetProcAddressFunc)(const char* func_name),
	void (*FuncNotFoundCallback)(const char* func_name) = nullptr );
