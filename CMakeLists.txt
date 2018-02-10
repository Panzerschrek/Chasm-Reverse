cmake_minimum_required(VERSION 3.1)

project(PanzerChasm)

set(CMAKE_CXX_STANDARD 11)

if(MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D _CRT_SECURE_NO_WARNINGS /MP")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D _CRT_SECURE_NO_WARNINGS /MP")
endif()

add_subdirectory(PanzerChasm)

option(BUILD_TOOLS "Enable compilation of tools" YES)

if(BUILD_TOOLS)
	set(COMMON_FILES
		../Common/files.cpp
		../Common/files.hpp
	)

	set(COMMON_PALETTE
		../Common/palette.cpp
		../Common/palette.hpp
	)

	set(COMMON_TGA
		../Common/tga.cpp
		../Common/tga.hpp
	)

	add_executable(ArchiveDepacker
		ArchiveDepacker/main.cpp
		${COMMON_FILES}
	)

	add_executable(BlendtabAnalyzer
		BlendtabAnalyzer/main.cpp
		${COMMON_FILES}
		${COMMON_PALETTE}
		${COMMON_TGA}
	)

	add_executable(CarToTGAConverter
		CarToTGAConverter/main.cpp
		${COMMON_FILES}
		${COMMON_PALETTE}
		${COMMON_TGA}
	)

	add_executable(CelDepacker
		CelDepacker/main.cpp
		${COMMON_FILES}
		${COMMON_TGA}
	)

	add_executable(FloorsExtractor
		FloorsExtractor/main.cpp
		${COMMON_FILES}
		${COMMON_PALETTE}
		${COMMON_TGA}
	)

	add_executable(MapToTGAConverter
		MapToTGAConverter/main.cpp
		${COMMON_FILES}
		${COMMON_TGA}
	)

	add_executable(ObjToTGAConverter
		ObjToTGAConverter/main.cpp
		${COMMON_FILES}
		${COMMON_PALETTE}
		${COMMON_TGA}
	)

	add_executable(PaletteExtractor
		PaletteExtractor/main.cpp
		${COMMON_FILES}
		${COMMON_PALETTE}
		${COMMON_TGA}
	)
endif(BUILD_TOOLS)
