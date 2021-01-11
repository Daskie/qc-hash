# qc-hash-performance

add_executable(qc-hash-performance performance-hash.cpp)
target_include_directories(qc-hash-performance SYSTEM PRIVATE $ENV{DEV_DIR}/qc-core/include)
target_link_libraries(qc-hash-performance PRIVATE qc-core qc-hash)

#qc-set-performance

add_executable(qc-set-performance performance-set.cpp)
target_include_directories(qc-set-performance SYSTEM PRIVATE $ENV{DEV_DIR}/qc-core/include)
target_link_libraries(qc-set-performance PRIVATE qc-hash)
