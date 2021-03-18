cmake_minimum_required(VERSION 3.17)
project(qc-hash)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_DEBUG_POSTFIX d)

### QC-HASH ###

add_library(qc-hash INTERFACE)

target_include_directories(
    qc-hash
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_compile_options(qc-hash INTERFACE -Wall -Werror -Wextra -Wpedantic -Wno-strict-aliasing)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    target_compile_options(qc-hash INTERFACE /W4 /WX)
endif()

### SUBDIRECTORIES ###

# qc-hash-test
add_subdirectory(test EXCLUDE_FROM_ALL)

# qc-hash-performance
add_subdirectory(performance EXCLUDE_FROM_ALL)

### INSTALL ###

# General install and target setup
install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY include DESTINATION .)

# Export targets to create importable cmake file
install(
    EXPORT ${PROJECT_NAME}-targets
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION lib/cmake/${PROJECT_NAME}
)

# Allow library to be found via find_package
include(CMakePackageConfigHelpers)

# Create config.cmake files
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    INSTALL_DESTINATION lib/cmake/${PROJECT_NAME}
)

# Install config.cmake files
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)
