cmake_minimum_required(VERSION 3.17)

project(qc-hash)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_DEBUG_POSTFIX -d)

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

# See https://cmake.org/cmake/help/git-stage/guide/importing-exporting/index.html

include(GNUInstallDirs)

# General install and target setup
install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY include DESTINATION .)

# Export targets to create importable cmake file
install(
    EXPORT ${PROJECT_NAME}-targets
    NAMESPACE ${CMAKE_PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

# Allow library to be found via find_package
include(CMakePackageConfigHelpers)

# Create config.cmake files
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# Install config.cmake files
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)
