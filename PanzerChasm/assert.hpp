#pragma once
#include <cassert>

// Simple assert wrapper.
// If you wish disable asserts, or do something else redefine this macro.
#ifdef DEBUG
#define U_ASSERT(x) \
	assert(x)
#else
#define PC_ASSERT(x)
#endif

#define PC_UNUSED(x) (void)x
