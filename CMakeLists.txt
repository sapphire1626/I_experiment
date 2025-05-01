cmake_minimum_required(VERSION 3.10)

project(I_experiment)

file(GLOB SRC_FILES src/*.c)

add_compile_options(-Og)

foreach(SRC_FILE ${SRC_FILES})
    get_filename_component(BIN_NAME ${SRC_FILE} NAME_WE)
    add_executable(${BIN_NAME} ${SRC_FILE})
    target_link_libraries(${BIN_NAME} m)
endforeach()