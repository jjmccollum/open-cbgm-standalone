#Specify headers directories:
set(SQLITE3_HEADERS ${PROJECT_SOURCE_DIR}/lib/sqlite3)
set(OPENCBGM_HEADERS ${PROJECT_SOURCE_DIR}/lib/open-cbgm/include)

# Add all executable scripts to be generated:
add_executable(populate_db populate_db.cpp)
add_executable(compare_witnesses compare_witnesses.cpp)
add_executable(find_relatives find_relatives.cpp)
add_executable(optimize_substemmata optimize_substemmata.cpp)
add_executable(print_local_stemma print_local_stemma.cpp)
add_executable(print_textual_flow print_textual_flow.cpp)
add_executable(print_global_stemma print_global_stemma.cpp)

# Point the executables to their include directories:
target_include_directories(populate_db PRIVATE ${SQLITE3_HEADERS} ${OPENCBGM_HEADERS})
target_include_directories(compare_witnesses PRIVATE ${SQLITE3_HEADERS} ${OPENCBGM_HEADERS})
target_include_directories(find_relatives PRIVATE ${SQLITE3_HEADERS} ${OPENCBGM_HEADERS})
target_include_directories(optimize_substemmata PRIVATE ${SQLITE3_HEADERS} ${OPENCBGM_HEADERS})
target_include_directories(print_local_stemma PRIVATE ${SQLITE3_HEADERS} ${OPENCBGM_HEADERS})
target_include_directories(print_textual_flow PRIVATE ${SQLITE3_HEADERS} ${OPENCBGM_HEADERS})
target_include_directories(print_global_stemma PRIVATE ${SQLITE3_HEADERS} ${OPENCBGM_HEADERS})

# Link the build targets to external libraries:
target_link_libraries(populate_db PUBLIC roaring pugixml sqlite3 open-cbgm)
target_link_libraries(compare_witnesses PUBLIC roaring pugixml sqlite3 open-cbgm)
target_link_libraries(find_relatives PUBLIC roaring pugixml sqlite3 open-cbgm)
target_link_libraries(optimize_substemmata PUBLIC roaring pugixml sqlite3 open-cbgm)
target_link_libraries(print_local_stemma PUBLIC roaring pugixml sqlite3 open-cbgm)
target_link_libraries(print_textual_flow PUBLIC roaring pugixml sqlite3 open-cbgm)
target_link_libraries(print_global_stemma PUBLIC roaring pugixml sqlite3 open-cbgm)

# Make sure the build targets are compiled with C++11:
target_compile_features(populate_db PRIVATE cxx_std_11)
target_compile_features(compare_witnesses PRIVATE cxx_std_11)
target_compile_features(find_relatives PRIVATE cxx_std_11)
target_compile_features(optimize_substemmata PRIVATE cxx_std_11)
target_compile_features(print_local_stemma PRIVATE cxx_std_11)
target_compile_features(print_textual_flow PRIVATE cxx_std_11)
target_compile_features(print_global_stemma PRIVATE cxx_std_11)
