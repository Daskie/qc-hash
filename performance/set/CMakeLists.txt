### DEPENDENCIES ###############################################################

find_package(qc-core REQUIRED)

### QC-HASH-PERFORMANCE-SET ####################################################

file(GLOB SRC_FILES *.cpp)

add_executable(qc-hash-performance-set ${SRC_FILES})

if(MSVC)
    target_compile_options(qc-hash-performance-set PRIVATE /W4)
else()
    message(FATAL_ERROR "TODO")
endif()

target_include_directories(qc-hash-performance-set SYSTEM PRIVATE C:/dev/internal/qc-core/include)

target_link_libraries(qc-hash-performance-set PRIVATE qc-hash)
