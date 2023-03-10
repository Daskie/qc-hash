cmake_minimum_required(VERSION 3.23)

project(qc-hash)

# Retrieve common CMake stuff
include(FetchContent)
FetchContent_Declare(qc-cmake GIT_REPOSITORY https://github.com/Daskie/qc-cmake.git)
FetchContent_MakeAvailable(qc-cmake)

set(QC_CXX_STANDARD 20)

qc_setup_target(qc-hash INTERFACE_LIBRARY)

add_subdirectory(test)

# TEMPORARILY OUT OF COMMISSION
# WILL SWITCH TO GOOGLE BENCHMARK SOON
#add_subdirectory(benchmark EXCLUDE_FROM_ALL)

qc_setup_install(TARGETS qc-hash)
