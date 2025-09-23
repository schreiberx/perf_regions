# Examples subdirectory CMakeLists.txt

# Function to create an instrumented executable target
function(add_perf_regions_example target_name source_files)
    # Parse arguments
    cmake_parse_arguments(ARG "" "" "SOURCES;DEPENDENCIES" ${ARGN})
    
    # Create the regular executable (excluded from ALL so never built by default)
    add_executable(${target_name} EXCLUDE_FROM_ALL ${source_files})
    target_link_libraries(${target_name} PRIVATE perf_regions::perf_regions)
    
    if(ARG_DEPENDENCIES)
        add_dependencies(${target_name} ${ARG_DEPENDENCIES})
    endif()
    
    # Create the instrumented version (also excluded from ALL)
    set(instrumented_target "${target_name}_perf_region")
    add_executable(${instrumented_target} EXCLUDE_FROM_ALL ${source_files})
    target_link_libraries(${instrumented_target} PRIVATE perf_regions::perf_regions)
    
    # Add custom command to run preprocessing and cleanup only when building examples
    if(Python3_Interpreter_FOUND)
        add_custom_command(TARGET ${instrumented_target} PRE_BUILD
            COMMAND ${Python3_EXECUTABLE} perf_regions_instrumentation.py preprocess
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${target_name}
            COMMENT "Running perf_regions instrumentation preprocessing for ${target_name}"
        )

        add_custom_command(TARGET ${instrumented_target} POST_BUILD
            COMMAND ${Python3_EXECUTABLE} perf_regions_instrumentation.py cleanup
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${target_name}
            COMMENT "Cleaning up perf_regions instrumentation for ${target_name}"
        )
    endif()
    
    if(ARG_DEPENDENCIES)
        add_dependencies(${instrumented_target} ${ARG_DEPENDENCIES})
    endif()
endfunction()

# Find Python for instrumentation scripts lazily; not required for core build
find_package(Python3 COMPONENTS Interpreter)

# Add each example subdirectory
add_subdirectory(array_test_c EXCLUDE_FROM_ALL)
add_subdirectory(array_test_f EXCLUDE_FROM_ALL)
add_subdirectory(stream_benchmark EXCLUDE_FROM_ALL)
add_subdirectory(nemo_test_f EXCLUDE_FROM_ALL)
add_subdirectory(nested_test_f EXCLUDE_FROM_ALL)

# Create a target to build all examples
add_custom_target(examples
    DEPENDS 
        array_test array_test_perf_region
        array_test_f array_test_f_perf_region
        stream_test stream_test_perf_region
        nemo_test nemo_test_perf_region
        nested_test nested_test_perf_region
)

# Create test targets (optional)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
    if(BUILD_TESTING)
        # Add tests for each example
        add_test(NAME array_test_c_run
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/array_test_c/run_tests.sh
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/array_test_c
        )
        
        add_test(NAME array_test_f_run
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/array_test_f/run_tests.sh
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/array_test_f
        )
        
        add_test(NAME stream_benchmark_run
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/stream_benchmark/run_tests.sh
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/stream_benchmark
        )
        
        add_test(NAME nemo_test_f_run
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/nemo_test_f/run_tests.sh
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/nemo_test_f
        )
        
        add_test(NAME nested_test_f_run
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/nested_test_f/run_tests.sh
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/nested_test_f
        )
    endif()
endif()