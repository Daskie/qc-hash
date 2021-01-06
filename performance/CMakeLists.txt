include(FetchContent)

FetchContent_Declare(
        qc-core
        SOURCE_DIR $ENV{DEV_DIR}/qc-core)

FetchContent_MakeAvailable(qc-core)

# qc-hash-performance

add_executable(qc-hash-performance performance-hash.cpp)

target_link_libraries(qc-hash-performance PRIVATE qc-core qc-hash)

#qc-set-performance

add_executable(qc-set-performance performance-set.cpp)

target_link_libraries(qc-set-performance PRIVATE qc-core qc-hash)
