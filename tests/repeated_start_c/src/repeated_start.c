#include <perf_regions.h>

int main(void)
{
	perf_regions_init();

	perf_region_start(0, "first");
	perf_region_stop(0);

	/* The second name must not replace the name stored on the first start. */
	perf_region_start(0, "second");
	perf_region_stop(0);

	perf_regions_finalize();
	return 0;
}
