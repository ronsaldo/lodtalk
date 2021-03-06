# Loden cmake build script.
cmake_minimum_required(VERSION 2.8)
project(Lodtalk)

# Check the build type
if (CMAKE_BUILD_TYPE STREQUAL "")
	# CMake defaults to leaving CMAKE_BUILD_TYPE empty. This screws up
	# differentiation between debug and release builds.
	set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build, options are: None (CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif ()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Output paths.
set(CMAKE_DEBUG_POSTFIX "_d")

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/dist")
set(Lodtalk_DATA_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/dist/data")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/;${CMAKE_MODULE_PATH}")

# Turn warnings and use C++ 2011.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	# using Visual Studio C++
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")

	# Do not deprecate the C runtime library.
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -pthread")

	# Export symbols from applications.
	#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--export-dynamic")
endif()

# Perform platform checks
include(${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
include(${CMAKE_ROOT}/Modules/CheckIncludeFileCXX.cmake)
include(${CMAKE_ROOT}/Modules/CheckFunctionExists.cmake)
include(${CMAKE_ROOT}/Modules/CheckLibraryExists.cmake)

# Find flex and bison
if(WIN32)
	list(APPEND CMAKE_PROGRAM_PATH "${CMAKE_SOURCE_DIR}/thirdparty/win_flex_bison")
endif()

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

# Set output dir.
set(EXECUTABLE_OUTPUT_PATH "${Lodtalk_BINARY_DIR}/dist")
set(LIBRARY_OUTPUT_PATH "${Lodtalk_BINARY_DIR}/dist")

# Add the current directory.
set(LODTALK_VM_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")
include_directories(
.
"${LODTALK_VM_INCLUDE_DIR}"
)

# Build the vm
add_subdirectory(vm)

# Build the apps
add_subdirectory(apps)
