#include <stdint.h>
#include <perf_regions.h>

int main(void)
{
	perf_regions_init();

	for (int i = 0; i < 5; i++)
	{
		perf_region_start(0, "papi_probe");

		volatile uint64_t value = 0;
		for (uint64_t j = 0; j < 100000; j++)
			value += j;

		perf_region_stop(0);
	}

	perf_regions_finalize();
	return 0;
}
