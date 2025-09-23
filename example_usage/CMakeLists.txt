# Example CMakeLists.txt for using perf_regions in your project

cmake_minimum_required(VERSION 3.16)
project(MyProject LANGUAGES C)

# Find perf_regions package
find_package(perf_regions REQUIRED)

# Create your executable
add_executable(my_program main.c)

# Link against perf_regions
target_link_libraries(my_program PRIVATE perf_regions::perf_regions)

# Set C standard
set_target_properties(my_program PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
)