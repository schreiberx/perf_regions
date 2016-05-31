#ifndef PERF_REGION_DEFINES_H
#define PERF_REGION_DEFINES_H



#define COMMENT1	"Identifiers for each region. Don't forget to add a line in performance_region.c !"
#define PERF_REGIONS_FOO		(1)
#define PERF_REGIONS_BAR		(2)
#define PERF_REGIONS_XYZ		(3)



#define COMMENT2	"Maximum number of regions"
#define PERF_REGIONS_MAX		(10)



#define COMMENT3	"Flags to determine which timing functionality should be used"
#define PERF_FLAG_TIMINGS		(65536*2)
#define PERF_FLAG_COUNTERS		(65536)



#endif
