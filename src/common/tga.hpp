#pragma once

namespace ChasmReverse
{

void WriteTGA(
	const unsigned short width, const unsigned short height,
	const unsigned char* const data,
	const unsigned char* const palette,
	const char* const file_name );

} // namespace ChasmReverse
