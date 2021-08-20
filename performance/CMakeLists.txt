### DEPENDENCIES ###############################################################

find_package(qc-core CONFIG REQUIRED)

find_package(absl CONFIG REQUIRED)

### QC-HASH-PERFORMANCE-SET ####################################################

file(GLOB SRC_FILES *.cpp)

add_executable(qc-hash-performance-map ${SRC_FILES})

if(MSVC)
    target_compile_options(qc-hash-performance-map PRIVATE /W4)
else()
    message(FATAL_ERROR "TODO")
endif()

target_link_libraries(qc-hash-performance-map PRIVATE qc-hash qc-core::qc-core absl::flat_hash_set)
