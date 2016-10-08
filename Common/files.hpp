#pragma once
#include <cstdio>

namespace ChasmReverse
{

void FileRead( std::FILE* const file, void* buffer, const unsigned int size );
void FileWrite( std::FILE* const file, const void* buffer, const unsigned int size );

} // namespace ChasmReverse
