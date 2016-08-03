#ifndef PERF_REGION_DEFINES_H
#define PERF_REGION_DEFINES_H



#define COMMENT1	"Maximum number of regions"
#define PERF_REGIONS_MAX		(128)


#define COMMENT2	"Identifiers for each region. Don't forget to add a line in performance_region.c !"
#define PERF_REGIONS_OVERHEAD_TIMINGS		(PERF_REGIONS_MAX-1)
#define PERF_REGIONS_OVERHEAD_COUNTERS		(PERF_REGIONS_MAX-2)
#define PERF_REGIONS_OVERHEAD_TIMINGS_COUNTERS	(PERF_REGIONS_MAX-3)

#define COMMENT3	"Flags to determine which timing functionality should be used"
#define PERF_FLAG_TIMINGS		(65536*2)
#define PERF_FLAG_COUNTERS		(65536)



#endif
