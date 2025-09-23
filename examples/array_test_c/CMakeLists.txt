# Array test C example

# Create executable targets
add_executable(array_test array_test.c)
target_link_libraries(array_test PRIVATE perf_regions::perf_regions)

# Create instrumented version
add_executable(array_test_perf_region array_test.c)
target_link_libraries(array_test_perf_region PRIVATE perf_regions::perf_regions)

# Custom commands for instrumentation
add_custom_command(TARGET array_test_perf_region PRE_BUILD
    COMMAND ${Python3_EXECUTABLE} perf_regions_instrumentation.py cleanup
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Cleaning up previous instrumentation"
)

add_custom_command(TARGET array_test_perf_region PRE_BUILD
    COMMAND ${Python3_EXECUTABLE} perf_regions_instrumentation.py preprocess
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Preprocessing perf_regions instrumentation"
)

# Cleanup after build
add_custom_command(TARGET array_test_perf_region POST_BUILD
    COMMAND ${Python3_EXECUTABLE} perf_regions_instrumentation.py cleanup
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Cleaning up perf_regions instrumentation"
)

# Set C standard
set_target_properties(array_test array_test_perf_region PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)