### DEPENDENCIES ###############################################################

find_package(qc-core CONFIG REQUIRED)

find_package(absl CONFIG REQUIRED)

### QC-HASH-PERFORMANCE-SET ####################################################

file(GLOB SRC_FILES *.cpp)

add_executable(qc-hash-performance ${SRC_FILES})

target_compile_options(qc-hash-performance PRIVATE ${QC_WARNINGS_ERROR})

target_link_libraries(qc-hash-performance PRIVATE qc-hash qc-core::qc-core absl::flat_hash_set)
