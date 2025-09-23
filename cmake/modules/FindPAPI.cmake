# FindPAPI.cmake - Find PAPI library and headers
# This module defines:
#  PAPI_FOUND        - True if PAPI is found
#  PAPI_INCLUDE_DIRS - Include directories for PAPI
#  PAPI_LIBRARIES    - Libraries to link against
#  PAPI::PAPI        - Imported target for PAPI

# Look for the header file
find_path(PAPI_INCLUDE_DIR
    NAMES papi.h
    HINTS
        $ENV{PAPI_ROOT}/include
        $ENV{PAPI_DIR}/include
        /usr/include
        /usr/local/include
        /opt/papi/include
    PATH_SUFFIXES papi
)

# Look for the library
find_library(PAPI_LIBRARY
    NAMES papi
    HINTS
        $ENV{PAPI_ROOT}/lib
        $ENV{PAPI_DIR}/lib
        /usr/lib
        /usr/lib64
        /usr/local/lib
        /usr/local/lib64
        /opt/papi/lib
    PATH_SUFFIXES papi
)

# Handle the QUIETLY and REQUIRED arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PAPI
    REQUIRED_VARS PAPI_LIBRARY PAPI_INCLUDE_DIR
    VERSION_VAR PAPI_VERSION
)

if(PAPI_FOUND)
    set(PAPI_LIBRARIES ${PAPI_LIBRARY})
    set(PAPI_INCLUDE_DIRS ${PAPI_INCLUDE_DIR})
    
    # Create imported target
    if(NOT TARGET PAPI::PAPI)
        add_library(PAPI::PAPI UNKNOWN IMPORTED)
        set_target_properties(PAPI::PAPI PROPERTIES
            IMPORTED_LOCATION "${PAPI_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${PAPI_INCLUDE_DIR}"
        )
    endif()
endif()

# Mark variables as advanced
mark_as_advanced(PAPI_INCLUDE_DIR PAPI_LIBRARY)