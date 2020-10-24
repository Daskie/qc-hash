# qc-hash-test

add_executable(qc-hash-test test-hash.cpp test-map.cpp)

target_link_libraries(qc-hash-test PRIVATE gtest gtest_main qc-core qc-hash)

target_include_directories(qc-hash-test PRIVATE ${gtest_SOURCE_DIR}/include)
