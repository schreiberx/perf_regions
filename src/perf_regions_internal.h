#ifndef PERF_REGIONS_INTERNAL_H
#define PERF_REGIONS_INTERNAL_H

#include <sys/time.h>

#include "perf_regions_defines.h"

#if USE_MPI
#include <mpi.h>
#endif

/**
 * Internal structure for each performance region.
 *
 * This is deliberately kept out of the public header so that consumers do not
 * depend on the compile-time configuration of the library.
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

	// Start time value (seconds) when POSIX timers are available
	double region_enter_time_posix;

	// Fallback gettimeofday() value
	struct timeval start_time_value;

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
	char *region_name;
};

struct PerfRegions
{
	// verbosity level
	int verbosity;

	// skip the first N executions of each performance region
	int skip_n;

	// array with all regions
	struct PerfRegion *perf_regions_list;

	// number of allocated region slots in perf_regions_list
	int num_regions_allocated;

	// Measure wallclock time
	int use_wallclock_time;

#if PERF_TIMING_POSIX
	// Whether posix_clock_init() succeeded at runtime
	int use_posix_clock;
#endif

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

#endif
