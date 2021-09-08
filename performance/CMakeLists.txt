### DEPENDENCIES ###############################################################

find_package(qc-core CONFIG REQUIRED)

find_package(absl CONFIG REQUIRED)

### QC-HASH-PERFORMANCE ########################################################

qc_setup_target(
    qc-hash-performance
    EXECUTABLE
    PRIVATE_LINKS
        qc-hash
        qc-core::qc-core
        absl::flat_hash_set
)
