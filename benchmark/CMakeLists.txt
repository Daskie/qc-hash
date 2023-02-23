find_package(qc-core CONFIG REQUIRED)

find_package(absl CONFIG REQUIRED)

qc_setup_target(
    qc-hash-benchmark
    EXECUTABLE
    PRIVATE_LINKS
        qc-hash
        qc-core::qc-core
        absl::flat_hash_set
    ENABLE_EXCEPTIONS
)
