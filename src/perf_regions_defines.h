#ifndef PERF_REGION_DEFINES_H
#define PERF_REGION_DEFINES_H

/*
 * "Maximum number of regions"
 */
#define PERF_REGIONS_MAX		(128)

/*
 * "Maximum number of performance counters"
 */
#define PERF_COUNTERS_MAX		(256)

/*
 * precompiler directives to avoid compiling with timing or perf counters
 *
 * This gets handy if perf counter PAPI is not available on certain systems.
 */
#ifndef PERF_REGIONS_USE_PAPI
#	define PERF_REGIONS_USE_PAPI		1
#endif


/***********************************************
 * Activate support for nested performance counters.
 * Warning: This leads to additional overheads and
 * might result in decreased accurate results!
 *
 ***********************************************
 * Recursion not activated:
 *
 *  perf_region_start();
 *    -> count_start();
 *       (reset and start counters)
 *
 *  perf_region_stop();
 *    -> count_stop();
 *       (stop counters)
 *    -> accumulate performance counters
 *
 *
 ***********************************************
 * Recursion activated:
 *
 *  perf_region_init();
 *       (reset and start counters)
 *
 *  perf_region_start();
 *    if num_nested_performance_regions == 0:
 *        -> count_start();
 *
 *    else:
 *        -> count_read_and_reset();
 *           (read counters)
 *           (update performance counters of outer regions)
 *    num_nested_performance_regions++;
 *
 *
 *  perf_region_stop();
 *    num_nested_performance_regions--;
 *
 *    if num_nested_performance_regions == 0:
 *       -> count_stop();
 *          (stop counters)
 *
 *       -> accumulate performance counters
 *
 *    else:
 *       -> count_read_and_reset();
 *          (read counters)
 *          (update performance counters of outer regions)
 *
 *       -> accumulate performance counters		/// overheads included here for nested functions
 */
#ifndef PERF_COUNTERS_NESTED
#define PERF_COUNTERS_NESTED		1
#endif


/*
 * Use the higher-resolution POSIX clock rather than just gettimeofday()
 */
#ifndef PERF_TIMING_POSIX
#define PERF_TIMING_POSIX               1
#endif


#ifndef PERF_DEBUG
#	define PERF_DEBUG		1
#endif

#ifndef USE_MPI
#define USE_MPI 1
#endif

#endif
