#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "perf_regions_defines.h"


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
	const char* i_region_name	///< type of measurements
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
void perf_regions_output_human_readable_text();

/**
 * Output csv file with information about each performance region
 */
void perf_regions_output_csv();


void perf_regions_reduce(int communicator);

/**
 * DECONSTRUCTOR
 */
void perf_regions_finalize();


