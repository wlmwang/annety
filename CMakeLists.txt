CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(annety C CXX)

# Build the testing codes.
# ENABLE_TESTING()

# Generate compile flags.
SET(CXX_FLAGS
	-g
	-std=c++11
	-fpic
	-Wall
	-Wextra
	-Werror
	-Wno-unused-parameter
	-Wold-style-cast
	-Woverloaded-virtual
	-Wpointer-arith
	-Wwrite-strings
	#-Wconversion
	#-Wshadow
	-rdynamic
	-march=native
	-DCHECK_PTHREAD_RETURN_VALUE
	-D_FILE_OFFSET_BITS=64
	-MMD
)
# For MACOSX platform.
IF (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	LIST(APPEND CXX_FLAGS "-Wno-null-dereference")
	LIST(APPEND CXX_FLAGS "-Wno-sign-conversion")
	LIST(APPEND CXX_FLAGS "-Wno-unused-local-typedef")
	LIST(APPEND CXX_FLAGS "-Wthread-safety")
	LIST(REMOVE_ITEM CXX_FLAGS "-rdynamic")
ENDIF()

# Set the cc compile flags.
STRING(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

# Set the cc compile debug/release.
IF (NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE "Release")
ENDIF()

SET(CMAKE_CXX_FLAGS_DEBUG "-O0")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
STRING(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
MESSAGE(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

# Set the object direcrory.
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

# Set headers file path.
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR} annety/include)

# Set recursive subdirectories.
ADD_SUBDIRECTORY(src)


# Only build examples if this is the main project.
IF (CMAKE_PROJECT_NAME STREQUAL "annety")
	OPTION(ANNETY_BUILD_EXAMPLES "Build Annety examples" ON)
ENDIF()

IF (ANNETY_BUILD_EXAMPLES)
	ADD_SUBDIRECTORY(examples)
ENDIF()
