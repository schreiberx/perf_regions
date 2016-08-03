#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "perf_region_defines.h"


/**
 * CONSTRUCTUR:
 *
 * initialize the performance regions, e.g. allocate the data structures
 */
void perf_regions_init();



/**
 * start measuring the performance for region given by the ID.
 */
void perf_region_start(
	int i_region_id,	///< id of region
	int i_measure_type	///< type of measurements
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
void perf_regions_output(FILE *s);



/**
 * Override normalization value for given region
 */
void perf_region_set_normalize(
		int i_region_id,			///< unique id of region
		double i_normalize_denom
);


/**
 * DECONSTRUCTOR
 */
void perf_regions_finalize();


