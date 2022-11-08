cmake_minimum_required(VERSION 3.23)

project(qc-hash)

# Retrieve common CMake stuff
include(FetchContent)
FetchContent_Declare(qc-cmake GIT_REPOSITORY https://github.com/Daskie/qc-cmake.git)
FetchContent_MakeAvailable(qc-cmake)

### QC-HASH ####################################################################

qc_setup_target(qc-hash INTERFACE_LIBRARY)

### SUBDIRECTORIES #############################################################

add_subdirectory(test EXCLUDE_FROM_ALL)

add_subdirectory(benchmark EXCLUDE_FROM_ALL)

### INSTALL ####################################################################

qc_setup_install(TARGETS qc-hash)
