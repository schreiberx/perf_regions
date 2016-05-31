#include <stdio.h>
#include <stdlib.h>
#include <perf_regions.h>
#include <perf_region_names.h>



int main(int i_argi, char **argv)
{
	for (int size = 1; size < 1024*1024*128/8; size*=2)
	{
		double *a = malloc(sizeof(double)*size);
		int iters = (1024*128)/size;
		iters = (iters <= 0 ? 0 : iters);
		iters = 128;

		perf_regions_init();

		printf("\n");
		printf("**************************************\n");
		printf("Mem size: %f KByte\n", (double)size*0.001);
		printf("Iterations: %i\n", iters);

		for (long long i = 0; i < size; i++)
			a[i] = i;

		for (int k = 0; k < iters; k++)
		{
			perf_region_start(PERF_REGIONS_FOO, PERF_FLAG_TIMINGS | PERF_FLAG_COUNTERS);
			for (long long i = 0; i < size; i++)
			{
				a[i] += a[i]*a[i];
			}
			perf_region_stop(PERF_REGIONS_FOO);
		}

		printf("**************************************\n");
		perf_regions_finalize();
		printf("**************************************\n");
		free(a);
	}
	return 0;
}
