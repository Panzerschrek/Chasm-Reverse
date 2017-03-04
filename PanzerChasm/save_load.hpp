#pragma once
#include "assert.hpp"

namespace PanzerChasm
{

struct SaveHeader
{
public:
	typedef unsigned int HashType;

	static const char c_expected_id[8];
	static constexpr unsigned int c_version= 0x100u; // Change each time, when format changed

public:
	static HashType CalculateHash( const unsigned char* data, unsigned int data_size );

public:
	unsigned char id[8]; // must be equal to c_expected_id
	unsigned int version;
	unsigned int content_size;
	unsigned int content_hash;
};

SIZE_ASSERT( SaveHeader, 20u );


} // namespace PanzerChasm
