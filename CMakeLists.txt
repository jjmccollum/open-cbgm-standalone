cmake_minimum_required(VERSION 3.1)
# Set the project name and version:
project(open-cbgm-standalone 
	VERSION 1.2.2
	DESCRIPTION "Standalone command-line interface for the open-cbgm library"
	LANGUAGES C CXX)

# If the user does not specify a build type (e.g., "Debug"), then default to Release:
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# Set the compilation warning flags:
set(WARNING_FLAGS "-Wall")
if(NOT MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

# Only do these if this is the main project, and not if it is included through add_subdirectory:
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	set(CMAKE_CXX_STANDARD 11)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
	set(CMAKE_CXX_EXTENSIONS OFF)
endif()

# Add the executables directory:
add_subdirectory(scripts)
# Add the sqlite3 source code directory:
add_subdirectory(lib/sqlite3)
# Add the open-cbgm source code directory:
add_subdirectory(lib/open-cbgm)