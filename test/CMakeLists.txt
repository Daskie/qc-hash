### DEPENDENCIES ###############################################################

find_package(GTest CONFIG REQUIRED)

find_package(qc-core CONFIG REQUIRED)

### QC-HASH-TEST ###############################################################

file(GLOB SRC_FILES *.cpp)

add_executable(qc-hash-test ${SRC_FILES})

target_compile_options(qc-hash-test PRIVATE ${QC_WARNINGS_ERROR})

target_link_libraries(
    qc-hash-test
    PRIVATE
        qc-hash
        GTest::gtest_main
        qc-core::qc-core
)
