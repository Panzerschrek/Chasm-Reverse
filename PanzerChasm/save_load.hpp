#pragma once
#include "assert.hpp"
#include "fwd.hpp"

namespace PanzerChasm
{

struct SaveHeader
{
public:
	typedef unsigned int HashType;

	static const char c_expected_id[8];
	static constexpr unsigned int c_expected_version= 0x105u; // Change each time, when format changed.

public:
	static HashType CalculateHash( const unsigned char* data, unsigned int data_size );

public:
	unsigned char id[8]; // must be equal to c_expected_id
	unsigned int version;
	unsigned int content_size;
	unsigned int content_hash;
};

SIZE_ASSERT( SaveHeader, 20u );

// Returns true, if all ok
bool SaveData(
	const char* file_name,
	const SaveComment& save_comment,
	const SaveLoadBuffer& data );

// Returns true, if all ok
bool LoadData(
	const char* file_name,
	SaveLoadBuffer& out_data );

bool LoadSaveComment(
	const char* file_name,
	SaveComment& out_save_comment );

void GetSaveFileNameForSlot(
	unsigned int slot_number,
	char* out_file_name,
	unsigned int out_file_name_max_length );

void CreateSlotSavesDir();

} // namespace PanzerChasm
