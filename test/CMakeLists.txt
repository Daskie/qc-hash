include(FetchContent)

### DEPENDENCIES ###

# qc-core
find_package(qc-core REQUIRED)

# googletest
FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git)
FetchContent_MakeAvailable(googletest)

### QC-HASH-TEST-HASH ###

add_executable(qc-hash-test-hash test-hash.cpp)

target_link_libraries(
    qc-hash-test-hash
    PRIVATE
        qc-hash
        qc-core::qc-core
        gtest_main
)

### QC-HASH-TEST-MAP ###

add_executable(qc-hash-test-map test-map.cpp)

target_link_libraries(
    qc-hash-test-map
    PRIVATE
        qc-hash
        qc-core::qc-core
        gtest_main
)

### QC-HASH-TEST-MAP-CHUNK ###

add_executable(qc-hash-test-map-chunk test-map-chunk.cpp)

target_link_libraries(
    qc-hash-test-map-chunk
    PRIVATE
        qc-hash
        qc-core::qc-core
        gtest_main
)

### QC-HASH-TEST-MAP-ORIG ###

add_executable(qc-hash-test-map-orig test-map-orig.cpp)

target_link_libraries(
    qc-hash-test-map-orig
    PRIVATE
        qc-hash
        qc-core::qc-core
        gtest_main
)
