CMAKE_MINIMUM_REQUIRED(VERSION 2.8.5)

PROJECT(annety C CXX)

# Build the testing cases.
ENABLE_TESTING()

# Generate compile flags.
SET(CXX_FLAGS
	-g
	-std=c++11
	-fpic	# Convenient for dynamic library to link this static library
	-Wall
	-Wextra
	-Werror
	-Wno-unused-parameter
	-Wwrite-strings
	-Wold-style-cast
	-Wpointer-arith
	-Woverloaded-virtual
	#-Wconversion
	#-Wshadow
	-rdynamic
	-march=native
	-DCHECK_PTHREAD_RETURN_VALUE
	-D_FILE_OFFSET_BITS=64
	-MMD
)
# For Clang compiler. for example: MACOSX
IF (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	LIST(APPEND CXX_FLAGS "-Wno-null-dereference")
	LIST(APPEND CXX_FLAGS "-Wno-sign-conversion")
	LIST(APPEND CXX_FLAGS "-Wno-unused-local-typedef")
	LIST(APPEND CXX_FLAGS "-Wthread-safety")
	LIST(REMOVE_ITEM CXX_FLAGS "-rdynamic")
ENDIF()

# Set the cc compile flags.
STRING(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

# Set default the object to `release`.
IF (NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE "Release")
ENDIF()

# Print the cc compile flags.
SET(CMAKE_CXX_FLAGS_DEBUG "-O0")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
STRING(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
MESSAGE(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

# Set the object output path.
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

# Add headers/library file path.
INCLUDE_DIRECTORIES(src include)

# Add recursive subdirectories.
ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(include)

# Only build examples if this is the main project.
IF (CMAKE_PROJECT_NAME STREQUAL "annety")
	MESSAGE(STATUS "Build Annety examples/tests...")
	ADD_SUBDIRECTORY(examples)
	ADD_SUBDIRECTORY(tests)
ENDIF()
