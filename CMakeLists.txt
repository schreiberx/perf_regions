cmake_minimum_required(VERSION 3.16)

# Project definition
project(perf_regions
    VERSION 1.0.0
    DESCRIPTION "Performance regions library for profiling code sections"
    LANGUAGES C Fortran
)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build options
option(PERF_REGIONS_USE_PAPI "Enable PAPI performance counters" OFF)
option(PERF_REGIONS_USE_MPI "Enable MPI support" ON)
# Examples are available via explicit target only; exclude from default/all
option(PERF_REGIONS_BUILD_EXAMPLES "Enable examples configuration (targets are excluded from ALL; build with 'make examples')" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Set default build type to Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# Compiler flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(PERF_DEBUG=1)
else()
    add_compile_definitions(PERF_DEBUG=0)
endif()

# Add compiler flags for better code quality
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -Wextra -fPIC)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3)
    else()
        add_compile_options(-g)
    endif()
endif()

# Find dependencies
if(PERF_REGIONS_USE_MPI)
    find_package(MPI REQUIRED COMPONENTS C Fortran)
    add_compile_definitions(USE_MPI=1)
else()
    add_compile_definitions(USE_MPI=0)
endif()

if(PERF_REGIONS_USE_PAPI)
    # Add custom FindPAPI module path
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
    find_package(PAPI REQUIRED)
    add_compile_definitions(PERF_REGIONS_USE_PAPI=1)
else()
    add_compile_definitions(PERF_REGIONS_USE_PAPI=0)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Install directory configuration
include(GNUInstallDirs)

# Add subdirectories
add_subdirectory(src)

# Always add examples but exclude from the default/ALL target so they are never
# built implicitly (including during 'make install'). They can still be built
# explicitly via the 'examples' target.
add_subdirectory(examples EXCLUDE_FROM_ALL)

# Install rules

# Export targets
install(EXPORT perf_regionsTargets
    FILE perf_regionsTargets.cmake
    NAMESPACE perf_regions::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/perf_regions
)

# Create config files
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/perf_regionsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/perf_regionsConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/perf_regions
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/perf_regionsConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/perf_regionsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/perf_regionsConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/perf_regions
)

# Install the perf_regions.py script
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/perf_regions.py"
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "perf_regions")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "perf_regions developers")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
include(CPack)

# Print configuration summary
message(STATUS "")
message(STATUS "perf_regions ${PROJECT_VERSION} configuration summary:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "  Fortran compiler: ${CMAKE_Fortran_COMPILER_ID} ${CMAKE_Fortran_COMPILER_VERSION}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  PAPI support: ${PERF_REGIONS_USE_PAPI}")
message(STATUS "  MPI support: ${PERF_REGIONS_USE_MPI}")
message(STATUS "  Examples: available via 'make examples' (excluded from ALL)")
message(STATUS "  Build shared libs: ${BUILD_SHARED_LIBS}")
message(STATUS "")