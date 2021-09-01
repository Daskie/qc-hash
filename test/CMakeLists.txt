### DEPENDENCIES ###############################################################

find_package(GTest CONFIG REQUIRED)

find_package(qc-core CONFIG REQUIRED)

### QC-HASH-TEST ###############################################################

qc_setup_target(
    qc-hash-test
    EXECUTABLE
    SOURCE_FILES
        *.cpp
    PRIVATE_LINKS
        qc-hash
        GTest::gtest_main
        qc-core::qc-core
)
