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
        $<INSTALL_INTERFACE:${PROJECT_NAME}/include>
)

### SUBDIRECTORIES #############################################################

add_subdirectory(test EXCLUDE_FROM_ALL)

add_subdirectory(performance EXCLUDE_FROM_ALL)

### INSTALL ####################################################################

# Install into named subdirectory on Windows
if(WIN32)
    set(DEST_DIR ${PROJECT_NAME})
else()
    set(DEST_DIR .)
endif()

# General install and target setup
install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
    LIBRARY DESTINATION ${DEST_DIR}/lib
    ARCHIVE DESTINATION ${DEST_DIR}/lib
    RUNTIME DESTINATION ${DEST_DIR}/bin
    INCLUDES DESTINATION ${DEST_DIR}/include
)

# Install headers
install(DIRECTORY include DESTINATION ${DEST_DIR})

# Export targets to create importable cmake file
install(
    EXPORT ${PROJECT_NAME}-targets
    NAMESPACE ${CMAKE_PROJECT_NAME}::
    DESTINATION ${DEST_DIR}/share/cmake/${PROJECT_NAME})

# Allow library to be found via find_package
include(CMakePackageConfigHelpers)

# Create config.cmake files
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    INSTALL_DESTINATION ${DEST_DIR}/share/cmake/${PROJECT_NAME}
)

# Install config.cmake files
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    DESTINATION ${DEST_DIR}/share/cmake/${PROJECT_NAME}
)
