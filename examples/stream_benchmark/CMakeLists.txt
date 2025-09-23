# Stream benchmark example

# Create executable targets (stream.c already includes mysecond function)
add_executable(stream_test stream.c)
target_link_libraries(stream_test PRIVATE perf_regions::perf_regions)

# Create instrumented version
add_executable(stream_test_perf_region stream.c)
target_link_libraries(stream_test_perf_region PRIVATE perf_regions::perf_regions)

# Custom commands for instrumentation
add_custom_command(TARGET stream_test_perf_region PRE_BUILD
    COMMAND ${Python3_EXECUTABLE} perf_regions_instrumentation.py cleanup
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Cleaning up previous instrumentation"
)

add_custom_command(TARGET stream_test_perf_region PRE_BUILD
    COMMAND ${Python3_EXECUTABLE} perf_regions_instrumentation.py preprocess
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Preprocessing perf_regions instrumentation"
)

# Cleanup after build
add_custom_command(TARGET stream_test_perf_region POST_BUILD
    COMMAND ${Python3_EXECUTABLE} perf_regions_instrumentation.py cleanup
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Cleaning up perf_regions instrumentation"
)

# Set C standard
set_target_properties(stream_test stream_test_perf_region PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)