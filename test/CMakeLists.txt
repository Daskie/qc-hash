include(FetchContent)

FetchContent_Declare(googletest SOURCE_DIR $ENV{DEV_DIR}/googletest)
FetchContent_MakeAvailable(googletest)

# qc-hash-test

add_executable(qc-hash-test test-hash.cpp test-map.cpp)
target_include_directories(qc-hash-test SYSTEM PRIVATE $ENV{DEV_DIR}/qc-core/include)
target_include_directories(qc-hash-test PRIVATE ${gtest_SOURCE_DIR}/include)
target_link_libraries(qc-hash-test PRIVATE gtest gtest_main qc-hash)
