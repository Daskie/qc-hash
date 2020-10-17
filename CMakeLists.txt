cmake_minimum_required(VERSION 3.17)
project(qc-hash)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(extern)
add_subdirectory(performance)
add_subdirectory(test)

add_library(qc-hash INTERFACE)

target_include_directories(qc-hash INTERFACE include)
