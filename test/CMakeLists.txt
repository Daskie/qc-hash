include(FetchContent)

FetchContent_Declare(
        googletest
        SOURCE_DIR $ENV{DEV_DIR}/googletest)

FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
        qc-core
        SOURCE_DIR $ENV{DEV_DIR}/qc-core)

FetchContent_MakeAvailable(qc-core)

# qc-hash-test

add_executable(qc-hash-test test-hash.cpp test-map.cpp)

target_link_libraries(qc-hash-test PRIVATE gtest gtest_main qc-core qc-hash)

target_include_directories(qc-hash-test PRIVATE ${gtest_SOURCE_DIR}/include)
