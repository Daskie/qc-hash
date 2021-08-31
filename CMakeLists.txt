cmake_minimum_required(VERSION 3.17)

project(qc-hash)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_DEBUG_POSTFIX -d)

# Retrieve common CMake stuff
include(FetchContent)
FetchContent_Declare(qc-cmake GIT_REPOSITORY https://github.com/Daskie/qc-cmake.git)
FetchContent_MakeAvailable(qc-cmake)

### QC-HASH ####################################################################

add_library(qc-hash INTERFACE)

target_include_directories(
    qc-hash
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

### SUBDIRECTORIES #############################################################

add_subdirectory(test EXCLUDE_FROM_ALL)

add_subdirectory(performance EXCLUDE_FROM_ALL)

### INSTALL ####################################################################

qc_install()
