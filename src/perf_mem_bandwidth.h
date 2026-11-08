#ifndef PERF_MEM_BANDWIDTH_H
#define PERF_MEM_BANDWIDTH_H

#include "perf_regions_defines.h"

#if PERF_REGIONS_USE_PAPI

/**
 * A PAPI event that contributes to the consolidated memory transfer volume.
 *
 * The name is owned by the caller after perf_mem_bandwidth_detect() returns.
 * bytes_per_count may be negative for architectures where a correction event
 * has to be subtracted (for example A64FX).
 */
struct PerfMemBandwidthEvent
{
	char *name;
	double bytes_per_count;
};

/**
 * Detect the memory-channel events for the current architecture.
 *
 * Returns the number of detected events, 0 if none were found, or -1 on
 * error.  The caller takes ownership of the returned event names.
 */
int perf_mem_bandwidth_detect(
	struct PerfMemBandwidthEvent *events,
	int max_events,
	int verbosity
);

#endif

#endif
