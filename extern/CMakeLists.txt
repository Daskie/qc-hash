if(NOT TARGET gtest)
    add_subdirectory(googletest)
endif()

if(NOT TARGET qc-core)
    add_subdirectory(qc-core)
endif()
