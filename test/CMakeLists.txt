find_package(GTest REQUIRED)

find_package(qc-core REQUIRED)

qc_setup_target(
    qc-hash-test
    EXECUTABLE
    PRIVATE_LINKS
        qc-hash
        gtest_main
        qc-core::qc-core
)

qc_setup_target(
    qc-hash-test-exceptions
    EXECUTABLE
    PRIVATE_LINKS
        qc-hash
        gtest_main
        qc-core::qc-core
    ENABLE_EXCEPTIONS
)
