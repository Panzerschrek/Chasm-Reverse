#pragma once
#include <filesystem>
#include "assert.hpp"
#include "fwd.hpp"

const std::filesystem::path SAVES_DIR = "saves";

namespace PanzerChasm
{

struct SaveHeader
{
public:
	typedef unsigned int HashType;

	static const char c_expected_id[8];
	static constexpr unsigned int c_expected_version= 0x109u; // Change each time, when format changed.

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
	const std::filesystem::path& file_name,
	const SaveComment& save_comment,
	const SaveLoadBuffer& data );

// Returns true, if all ok
bool LoadData(
	const std::filesystem::path& file_name,
	SaveLoadBuffer& out_data );

bool LoadSaveComment(
	const std::filesystem::path& file_name,
	SaveComment& out_save_comment );

std::filesystem::path GetSaveFileNameForSlot( const uint8_t slot_number );
std::filesystem::path GetScreenShotFileNameForSlot( const uint8_t slot_number, const std::filesystem::path& shot_name = "shot" );
uint8_t GetScreenShotSlotNumber( const std::filesystem::path& saves_dir = SAVES_DIR );

void CreateSlotSavesDir();

} // namespace PanzerChasm
