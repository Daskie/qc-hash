add_executable(qc-hash-performance performance-hash.cpp)
target_link_libraries(qc-hash-performance PRIVATE qc-core qc-hash)
target_compile_options(qc-hash-performance PRIVATE -Wall -Werror -Wpedantic -Wno-strict-aliasing)

add_executable(qc-set-performance performance-set.cpp)
target_link_libraries(qc-set-performance PRIVATE qc-core qc-hash)
target_compile_options(qc-set-performance PRIVATE -Wall -Werror -Wpedantic)
