# Source files for the perf_regions library
set(PERF_REGIONS_SOURCES
    perf_regions.c
    posix_clock.c
    papi_counters.c
    perf_regions_fortran.F90
)

set(PERF_REGIONS_HEADERS
    perf_regions.h
    perf_regions_defines.h
)

# Create the shared library
add_library(perf_regions ${PERF_REGIONS_SOURCES})

# Add alias for namespace usage
add_library(perf_regions::perf_regions ALIAS perf_regions)

# Set target properties
set_target_properties(perf_regions PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Include directories
target_include_directories(perf_regions
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/perf_regions>
)

# Link dependencies
if(PERF_REGIONS_USE_MPI)
    target_link_libraries(perf_regions PUBLIC MPI::MPI_C MPI::MPI_Fortran)
endif()

if(PERF_REGIONS_USE_PAPI)
    target_link_libraries(perf_regions PUBLIC PAPI::PAPI)
endif()

# Fortran module output directory
set_target_properties(perf_regions PROPERTIES
    Fortran_MODULE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/modules
)

# Make sure Fortran modules are available during build
target_include_directories(perf_regions
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/modules>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/perf_regions>
)

# Install the library and headers
install(TARGETS perf_regions
    EXPORT perf_regionsTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install headers manually
install(FILES ${PERF_REGIONS_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/perf_regions
)

# Install Fortran modules
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/modules/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/perf_regions
    FILES_MATCHING PATTERN "*.mod"
)