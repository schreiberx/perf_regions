#ifndef PERF_REGIONS_H
#define PERF_REGIONS_H

#include <stddef.h>

#include "perf_regions_defines.h"

#if USE_MPI
#include <mpi.h>
#endif

/*
 * The library's internal data structures are intentionally opaque to users of
 * this header.  They depend on compile-time options such as MPI/PAPI support,
 * so exposing them would make client code ABI-dependent on the exact library
 * configuration.
 */
struct PerfRegions;
extern struct PerfRegions perf_regions;

/**
 * CONSTRUCTOR:
 *
 * initialize the performance regions, e.g. allocate the data structures
 */
void perf_regions_init(void);

#if USE_MPI
/**
 * Call this from C code
 */
void perf_regions_init_mpi(MPI_Comm communicator);
/**
 * Call this from Fortran code
 */
void perf_regions_init_mpi_fortran(int communicator);
#endif

/**
 * start measuring the performance for region given by the ID.
 */
void perf_region_start(
	int i_region_id,	///< id of region
	const char *i_region_name	///< region name (associated with ID)
);

/**
 * Fortran entry point: the caller also passes the string length because
 * Fortran strings are not necessarily NUL-terminated.
 */
void perf_region_start_fortran(
	int i_region_id,
	const char *i_region_name,
	size_t len
);

/**
 * stop measuring the performance for region given by the ID
 */
void perf_region_stop(
	int i_region_id		///< unique id of region
);

/**
 * Output information on all performance counters
 */
void perf_regions_output_human_readable_text(void);

/**
 * DECONSTRUCTOR
 */
void perf_regions_finalize(void);

#endif
