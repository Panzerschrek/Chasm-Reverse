cmake_minimum_required(VERSION 3.1)

project(Chasm-Reverse)

set(CMAKE_CXX_STANDARD 11)

if(MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D _CRT_SECURE_NO_WARNINGS /MP")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D _CRT_SECURE_NO_WARNINGS /MP")
endif()

option(BUILD_TOOLS "Enable compilation of tools" YES)
include(CheckCXXSourceCompiles)
include(GNUInstallDirs)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(SDL2 REQUIRED)

include_directories(${SDL2_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/src/
	${CMAKE_CURRENT_SOURCE_DIR}/src/panzer_ogl_lib)

# Source and header files
file(GLOB_RECURSE CHASM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE CHASM_HEADERS 
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.inl")

# Detect MMX support

set(SAFE_CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmmx")
endif()

CHECK_CXX_SOURCE_COMPILES("#include <mmintrin.h>
	int main(void) { __m64 v = _m_from_int(0); }"
	HAVE_MMX)

if(HAVE_MMX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPC_MMX_INSTRUCTIONS")
else()
	set(CMAKE_CXX_FLAGS "${SAFE_CMAKE_CXX_FLAGS}")
endif()

# Configure libraries

set(CHASM_LIBS
	${SDL2_LIBRARY}
)

if(WIN32)
	set(CHASM_LIBS
		${CHASM_LIBS}
		opengl32
		ws2_32
	)
	set(CHASM_RESOURCES
		res/PanzerChasm.rc
	)
else()
	set(CHASM_LIBS
		${CHASM_LIBS}
		GL
	)
endif()

# Configure executable


add_executable(PanzerChasm ${CHASM_SOURCES} ${CHASM_HEADERS} ${CHASM_RESOURCES})
target_link_libraries(PanzerChasm ${CHASM_LIBS})

if(BUILD_TOOLS)
file(GLOB_RECURSE COMMON_FILES src/common/files.*) 
file(GLOB_RECURSE COMMON_PALETTE src/common/palette.*)
file(GLOB_RECURSE COMMON_TGA src/common/tga.*)

add_executable(ArchiveDepacker
	${CMAKE_CURRENT_SOURCE_DIR}/tools/ArchiveDepacker/main.cpp
	${COMMON_FILES}
)

add_executable(BlendtabAnalyzer
	${CMAKE_CURRENT_SOURCE_DIR}/tools/BlendtabAnalyzer/main.cpp
	${COMMON_FILES}
	${COMMON_PALETTE}
	${COMMON_TGA}
)

add_executable(CarToTGAConverter
	${CMAKE_CURRENT_SOURCE_DIR}/tools/CarToTGAConverter/main.cpp
	${COMMON_FILES}
	${COMMON_PALETTE}
	${COMMON_TGA}
)

add_executable(CelDepacker
	${CMAKE_CURRENT_SOURCE_DIR}/tools/CelDepacker/main.cpp
	${COMMON_FILES}
	${COMMON_TGA}
)

add_executable(FloorsExtractor
	${CMAKE_CURRENT_SOURCE_DIR}/tools/FloorsExtractor/main.cpp
	${COMMON_FILES}
	${COMMON_PALETTE}
	${COMMON_TGA}
)

add_executable(MapToTGAConverter
	${CMAKE_CURRENT_SOURCE_DIR}/tools/MapToTGAConverter/main.cpp
	${COMMON_FILES}
	${COMMON_TGA}
)

add_executable(ObjToTGAConverter
	${CMAKE_CURRENT_SOURCE_DIR}/tools/ObjToTGAConverter/main.cpp
	${COMMON_FILES}
	${COMMON_PALETTE}
	${COMMON_TGA}
)

add_executable(PaletteExtractor
	${CMAKE_CURRENT_SOURCE_DIR}/tools/PaletteExtractor/main.cpp
	${COMMON_FILES}
	${COMMON_PALETTE}
	${COMMON_TGA}
)
endif(BUILD_TOOLS)

