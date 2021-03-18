### DEPENDENCIES ###

# qc-core
find_package(qc-core CONFIG REQUIRED)

### QC-HASH-PERFORMANCE-HASH ###

add_executable(qc-hash-performance-hash performance-hash.cpp)

target_link_libraries(
    qc-hash-performance-hash
    PRIVATE
        qc-hash
        qc-core::qc-core
)

### QC-HASH-PERFORMANCE-SET ###

add_executable(qc-hash-performance-set performance-set.cpp)

target_link_libraries(
    qc-hash-performance-set
    PRIVATE
        qc-hash
        qc-core::qc-core
)
