cmake_minimum_required(VERSION 3.0)
# Set the project name and version:
project(open-cbgm-standalone 
	VERSION 1.3.1
	DESCRIPTION "Standalone command-line interface for the open-cbgm library"
	LANGUAGES C CXX)

# Set the compilation warning flags:
set(WARNING_FLAGS "-Wall")
# Set optimization flags for each build type:
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

# Set build options and locations if they have not been set already:
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS YES CACHE BOOL "Export all symbols")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib CACHE PATH "Path to build archive directory")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib CACHE PATH "Path to build library directory")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin CACHE PATH "Path to build runtime directory")

# Add the sqlite3 source code directory:
add_subdirectory(lib/sqlite3)
# Add the open-cbgm source code directory:
add_subdirectory(lib/open-cbgm)
# Add the executables directory:
add_subdirectory(scripts)