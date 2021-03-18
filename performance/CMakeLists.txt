### DEPENDENCIES ###

# qc-core
find_package(qc-core CONFIG REQUIRED)

### qc-hash-performance-hash ###

add_executable(qc-hash-performance-hash performance-hash.cpp)

target_link_libraries(qc-hash-performance-hash PRIVATE qc-hash qc-core::qc-core)

### qc-hash-performance-set ###

add_executable(qc-hash-performance-set performance-set.cpp)

target_link_libraries(qc-hash-performance-set PRIVATE qc-hash qc-core::qc-core)
