cmake_minimum_required(VERSION 3.17)
project(qc-hash)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(performance)
add_subdirectory(test)

# qc-hash

add_library(qc-hash INTERFACE)

target_include_directories(qc-hash INTERFACE include)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(qc-hash INTERFACE -Wall -Werror -Wextra -Wpedantic -Wno-strict-aliasing)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    target_compile_options(qc-hash INTERFACE /W4 /WX)
endif()
