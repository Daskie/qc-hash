cmake_minimum_required(VERSION 3.17)

project(qc-hash)

set(CMAKE_CXX_STANDARD 20)

# Retrieve common CMake stuff
include(FetchContent)
FetchContent_Declare(qc-cmake GIT_REPOSITORY https://github.com/Daskie/qc-cmake.git)
FetchContent_MakeAvailable(qc-cmake)

qc_setup_general()

### QC-HASH ####################################################################

qc_setup_target(
    qc-hash
    INTERFACE_LIBRARY
    INSTALLABLE
)

### SUBDIRECTORIES #############################################################

add_subdirectory(test EXCLUDE_FROM_ALL)

add_subdirectory(performance EXCLUDE_FROM_ALL)
