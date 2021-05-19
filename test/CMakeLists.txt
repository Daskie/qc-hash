### DEPENDENCIES ###

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

target_include_directories(qc-hash-test-fasthash SYSTEM PRIVATE C:/dev/internal/qc-core/include)

target_link_libraries(
    qc-hash-test-fasthash
    PRIVATE
        qc-hash
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

target_include_directories(qc-hash-test-map SYSTEM PRIVATE C:/dev/internal/qc-core/include)

target_link_libraries(
    qc-hash-test-map
    PRIVATE
        qc-hash
        GTest::gtest_main
)
