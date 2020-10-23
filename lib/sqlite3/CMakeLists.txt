# Find the library that supports multi-threading (preferably pthread):
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
# Combine the object source files into a library:
add_library(sqlite3 sqlite3.c)
# Point the library to its include directory:
target_include_directories(sqlite3 PUBLIC ${PROJECT_SOURCE_DIR}/lib/sqlite3)
# Set compiler flags for the library:
target_compile_definitions(sqlite3 PUBLIC SQLITE_OMIT_LOAD_EXTENSION=1)
# Link it to its dependencies:
target_link_libraries(sqlite3 Threads::Threads)