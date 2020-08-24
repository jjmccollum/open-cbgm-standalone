# Combine the object source files into a library:
add_library(sqlite3 STATIC sqlite3.c)

# Point the libraries to their include directories:
target_include_directories(sqlite3 PRIVATE .)

# Find the library that supports multi-threading (preferably pthread):
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Set compiler flags for sqlite3:
target_compile_definitions(sqlite3 PUBLIC SQLITE_OMIT_LOAD_EXTENSION=1)
target_link_libraries(sqlite3 Threads::Threads)