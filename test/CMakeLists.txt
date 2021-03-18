include(FetchContent)

### DEPENDENCIES ###

# qc-core
find_package(qc-core REQUIRED)

# googletest
FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git)
FetchContent_MakeAvailable(googletest)

### QC-HASH-TEST ###

file(GLOB qc-hash-test_SRC *.cpp)

add_executable(qc-hash-test ${qc-hash-test_SRC})

target_link_libraries(
    qc-hash-test
    PRIVATE
        qc-hash
        qc-core::qc-core
        gtest_main
)
