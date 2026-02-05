#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "perf_regions_defines.h"

#if USE_MPI
#include <mpi.h>
#endif


/**
 * Structure for each of the performance regions
 */
struct PerfRegion
{
#if PERF_REGIONS_USE_PAPI
	// storage for performance counter
	long long counter_values[PERF_COUNTERS_MAX];
#endif

	// accumulator to get total wallclock time spent in this region and its variance (over time)
	double counter_wallclock_time;
	double squared_dist_wallclock_time;
	double min_wallclock_time;
	double max_wallclock_time;
	double running_mean;

#ifdef PERF_TIMING_POSIX
	// Start time value (seconds)
	double region_enter_time_posix;
#else
	// time value
	struct timeval start_time_value;
#endif

	// region enter counter to normalize the performance counters
	double region_enter_counter;
	
	// counts the number of times this region measurement was skipped
	long long region_skipped_counter;

#if PERF_COUNTERS_NESTED
	// Set to true if performance counters could be spoiled due to nested
	int spoiled;
#endif

#if PERF_DEBUG
	// debug flag to assure that perf counters are not entering a region twice
	int active;
#endif

	// set to 1 if we only want to skip the current measurement
	int current_execution_skipped;

	// Region name
	char* region_name;
};


struct PerfRegions
{
	// verbosity level
	int verbosity;

	// skip the first N executions of each performance region
	int skip_n;

	// array with all regions
	struct PerfRegion *perf_regions_list;

	// Measure wallclock time
	int use_wallclock_time;

#if PERF_REGIONS_USE_PAPI
	// Measure with PAPI
	int use_papi;

	// names of performance counters
	char *perf_counter_names[PERF_COUNTERS_MAX];
#endif

	// number of performance counters
	int num_perf_counters;

#if PERF_COUNTERS_NESTED
	// number of nested performance regions
	int num_nested_performance_regions;

	// number of maximal nested performance regions
	#define PERF_NESTED_REGIONS_MAX 16

	struct PerfRegion *nested_performance_regions[PERF_NESTED_REGIONS_MAX];
#endif

	// the application uses MPI / perf_regions will be called for MPI processes
	// we want to do reduction at the end for the result
	int use_mpi;
#if USE_MPI
	MPI_Comm comm;
#endif

	// Output configuration
	int output_console;
	int output_json;
	char *output_json_filename;

};

extern struct PerfRegions perf_regions;

/**
 * CONSTRUCTUR:
 *
 * initialize the performance regions, e.g. allocate the data structures
 */
void perf_regions_init();



#if USE_MPI
#include <mpi.h>
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

/**
 * DECONSTRUCTOR
 */
void perf_regions_finalize();


