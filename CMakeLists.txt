cmake_minimum_required(VERSION 3.10)

project(I_experiment)

set(CMAKE_C_STANDARD 99)

file(GLOB SRC_FILES src/*.c)
file(GLOB LIB_SRC src/lib/*.c)

add_compile_options(-Og)

add_library(lib ${LIB_SRC})
target_include_directories(lib PUBLIC src/lib)
if(APPLE)
    target_include_directories(lib PUBLIC /opt/homebrew/include)
    target_link_libraries(lib /opt/homebrew/lib/libfftw3.dylib m)
elseif(UNIX)
    target_include_directories(lib PUBLIC /usr/include)
    target_link_libraries(lib /usr/lib/x86_64-linux-gnu/libfftw3.so m)
else()
    message(FATAL_ERROR "Unsupported platform"
        "Please modify the CMakeLists.txt to support your platform.")
endif()

foreach(SRC_FILE ${SRC_FILES})
    get_filename_component(BIN_NAME ${SRC_FILE} NAME_WE)
    add_executable(${BIN_NAME} ${SRC_FILE})
    target_link_libraries(${BIN_NAME} m lib)
endforeach()