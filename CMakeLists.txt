cmake_minimum_required(VERSION 3.24)

project(qc-hash)

# Fetch dependencies
if(${PROJECT_IS_TOP_LEVEL})
    include(FetchContent)

    FetchContent_Declare(qc-cmake SOURCE_DIR ${CMAKE_SOURCE_DIR}/../qc-cmake)

    FetchContent_Declare(qc-core SOURCE_DIR ${CMAKE_SOURCE_DIR}/../qc-core OVERRIDE_FIND_PACKAGE)

    option(BUILD_GMOCK "" OFF)
    option(INSTALL_GTEST "" OFF)
    option(gtest_force_shared_crt "" ON)
    FetchContent_Declare(GTest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG v1.14.0 OVERRIDE_FIND_PACKAGE)

    FetchContent_MakeAvailable(qc-cmake qc-core GTest)
endif()

set(QC_CXX_STANDARD 20)

qc_setup_target(qc-hash INTERFACE_LIBRARY)

if(${PROJECT_IS_TOP_LEVEL})
    add_subdirectory(test EXCLUDE_FROM_ALL)

    # TEMPORARILY OUT OF COMMISSION
    # WILL SWITCH TO GOOGLE BENCHMARK SOON
    #add_subdirectory(benchmark EXCLUDE_FROM_ALL)
endif()
