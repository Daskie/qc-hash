### DEPENDENCIES ###

# qc-core
find_package(qc-core REQUIRED)

# GTest
find_package(GTest CONFIG REQUIRED)

### QC-HASH-TEST-FASTHASH ###

add_executable(qc-hash-test-fasthash test-fasthash.cpp)

# Link executable statically
set_property(TARGET qc-hash-test-fasthash PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
    target_compile_options(qc-hash-test-fasthash PRIVATE /W4 /WX)
else()
    target_compile_options(qc-hash-test-fasthash PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_link_libraries(
    qc-hash-test-fasthash
    PRIVATE
        qc-hash
        qc-core::qc-core
        GTest::gtest_main
)

### QC-HASH-TEST-MAP ###

add_executable(qc-hash-test-map test-map.cpp)

# Link executable statically
set_property(TARGET qc-hash-test-map PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
    target_compile_options(qc-hash-test-map PRIVATE /W4 /WX)
else()
    target_compile_options(qc-hash-test-map PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_link_libraries(
    qc-hash-test-map
    PRIVATE
        qc-hash
        qc-core::qc-core
        GTest::gtest_main
)

### QC-HASH-TEST-MAP-CHUNK ###

add_executable(qc-hash-test-map-chunk test-map-chunk.cpp)

# Link executable statically
set_property(TARGET qc-hash-test-map-chunk PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
    target_compile_options(qc-hash-test-map-chunk PRIVATE /W4 /WX)
else()
    target_compile_options(qc-hash-test-map-chunk PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_link_libraries(
    qc-hash-test-map-chunk
    PRIVATE
        qc-hash
        qc-core::qc-core
        GTest::gtest_main
)

### QC-HASH-TEST-MAP-ORIG ###

add_executable(qc-hash-test-map-orig test-map-orig.cpp)

# Link executable statically
set_property(TARGET qc-hash-test-map-orig PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
    target_compile_options(qc-hash-test-map-orig PRIVATE /W4 /WX)
else()
    target_compile_options(qc-hash-test-map-orig PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_link_libraries(
    qc-hash-test-map-orig
    PRIVATE
        qc-hash
        qc-core::qc-core
        GTest::gtest_main
)
