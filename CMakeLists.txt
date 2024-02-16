cmake_minimum_required(VERSION 3.4)
# Set the project name and version:
project(open-cbgm-standalone 
	VERSION 1.7.0
	DESCRIPTION "Standalone command-line interface for the open-cbgm library"
	LANGUAGES C CXX)

include(CTest)

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

# Disable testing unless the BUILD_TESTS option is set:
option(BUILD_TESTS "Build open-cbgm-standalone unit tests" OFF)

# Add the sqlite3 source code directory:
add_subdirectory(lib/sqlite3)
# Add the open-cbgm source code directory:
add_subdirectory(lib/open-cbgm)
# Add the executables directory:
add_subdirectory(scripts)
# Optionally add the unit tests:
if (BUILD_TESTS)
  add_test(NAME populate_db COMMAND populate_db -z defective -z orthographic -s "*" -s T ../lib/open-cbgm/examples/test.xml test.db)
  add_test(NAME enumerate_relationships_unclear COMMAND enumerate_relationships test.db A B unclear)
  add_test(NAME enumerate_relationships_prior_posterior COMMAND enumerate_relationships test.db A B prior posterior)
  add_test(NAME enumerate_relationships_all COMMAND enumerate_relationships test.db A B)
  add_test(NAME compare_witnesses COMMAND compare_witnesses test.db A B)
  add_test(NAME compare_witnesses_all COMMAND compare_witnesses test.db A)
  add_test(NAME find_relatives_all_readings COMMAND find_relatives test.db A B00K0V0U4)
  add_test(NAME find_relatives_one_reading COMMAND find_relatives test.db A B00K0V0U4 a)
  add_test(NAME find_relatives_multiple_readings COMMAND find_relatives test.db A B00K0V0U4 a b)
  add_test(NAME optimize_substemmata COMMAND optimize_substemmata test.db E)
  add_test(NAME optimize_substemmata_within_bound COMMAND optimize_substemmata -b 5 test.db E)
  add_test(NAME print_local_stemma_all_passages COMMAND print_local_stemma test.db)
  add_test(NAME print_local_stemma_one_passage COMMAND print_local_stemma test.db B00K0V0U6)
  add_test(NAME print_local_stemma_multiple_passages COMMAND print_local_stemma test.db B00K0V0U6 B00K0V0U8)
  add_test(NAME print_local_stemma_weights COMMAND print_local_stemma --weights test.db)
  add_test(NAME print_textual_flow_all_passages COMMAND print_textual_flow test.db)
  add_test(NAME print_textual_flow_one_passage COMMAND print_textual_flow test.db B00K0V0U6)
  add_test(NAME print_textual_flow_multiple_passages COMMAND print_textual_flow test.db B00K0V0U6 B00K0V0U8)
  add_test(NAME print_textual_flow_strengths COMMAND print_textual_flow --strengths test.db)
  add_test(NAME print_global_stemma COMMAND print_global_stemma test.db)
  add_test(NAME print_global_stemma_lengths_strengths COMMAND print_global_stemma --lengths --strengths test.db)
endif()
