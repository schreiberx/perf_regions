#include <stddef.h>
#include <perf_regions.h>

int main(void)
{
	perf_regions_init();

	/* Grow the region table while an outer region is active. */
	perf_region_start(0, "outer");
	perf_region_start(200, "inner");
	perf_region_stop(200);
	perf_region_stop(0);

	/* Force another growth beyond the first extension. */
	perf_region_start(500, "late");
	perf_region_stop(500);

	perf_regions_finalize();
	return 0;
}
