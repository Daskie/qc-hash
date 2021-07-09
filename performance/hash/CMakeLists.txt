### DEPENDENCIES ###############################################################

find_package(qc-core CONFIG REQUIRED)

### QC-HASH-PERFORMANCE-HASH ###################################################

file(GLOB SRC_FILES *.cpp)

add_executable(qc-hash-performance-hash ${SRC_FILES})

if(MSVC)
    target_compile_options(qc-hash-performance-hash PRIVATE /W4)
else()
    message(FATAL_ERROR "TODO")
endif()

target_include_directories(qc-hash-performance-hash SYSTEM PRIVATE C:/dev/internal/qc-core/include)

target_link_libraries(qc-hash-performance-hash PRIVATE qc-hash)
