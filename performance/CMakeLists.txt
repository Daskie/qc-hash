### DEPENDENCIES ###

# qc-core
find_package(qc-core REQUIRED)

### QC-HASH-PERFORMANCE-HASH ###

add_executable(qc-hash-performance-hash performance-hash.cpp)

# Link executable statically
set_property(TARGET qc-hash-performance-hash PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
    target_compile_options(qc-hash-performance-hash PRIVATE /W4 /WX)
else()
    target_compile_options(qc-hash-performance-hash PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_link_libraries(
    qc-hash-performance-hash
    PRIVATE
        qc-hash
        qc-core::qc-core
)

### QC-HASH-PERFORMANCE-SET ###

add_executable(qc-hash-performance-set performance-set.cpp)

# Link executable statically
set_property(TARGET qc-hash-performance-set PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
    target_compile_options(qc-hash-performance-set PRIVATE /W4 /WX)
else()
    target_compile_options(qc-hash-performance-set PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

target_link_libraries(
    qc-hash-performance-set
    PRIVATE
        qc-hash
        qc-core::qc-core
)
