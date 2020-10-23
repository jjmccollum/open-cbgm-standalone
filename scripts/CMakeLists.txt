# Add all executable scripts to be generated:
add_executable(populate_db populate_db.cpp)
add_executable(compare_witnesses compare_witnesses.cpp)
add_executable(find_relatives find_relatives.cpp)
add_executable(optimize_substemmata optimize_substemmata.cpp)
add_executable(print_local_stemma print_local_stemma.cpp)
add_executable(print_textual_flow print_textual_flow.cpp)
add_executable(print_global_stemma print_global_stemma.cpp)

# Link the build targets to external libraries:
target_link_libraries(populate_db cxxopts sqlite3 open-cbgm)
target_link_libraries(compare_witnesses cxxopts sqlite3 open-cbgm)
target_link_libraries(find_relatives cxxopts sqlite3 open-cbgm)
target_link_libraries(optimize_substemmata cxxopts sqlite3 open-cbgm)
target_link_libraries(print_local_stemma cxxopts sqlite3 open-cbgm)
target_link_libraries(print_textual_flow cxxopts sqlite3 open-cbgm)
target_link_libraries(print_global_stemma cxxopts sqlite3 open-cbgm)
