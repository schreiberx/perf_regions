#include <stdio.h>
#include <stdlib.h>
#include <perf_regions.h>
#include <perf_region_names.h>


double *a;
int size;



void run_computations()
{
	for (long long i = 0; i < size; i++)
		a[i] += a[i]*a[i];
}



int main(int i_argi, char **argv)
{
	for (size = 1; size < 1024*1024*128/8; size*=2)
	{
		a = malloc(sizeof(double)*size);
		int iters = (1024*128)/size;
		iters = (iters <= 0 ? 2 : iters);
		iters = (iters > 128 ? 128 : iters);

		perf_regions_init();

		printf("\n");
		printf("**************************************\n");
		printf("Mem size: %f KByte\n", (double)size*0.001);
		printf("Iterations: %i\n", iters);

		// initialize everything
		for (size_t i = 0; i < size; i++)
			a[i] = i;

		// dummy computations to warmup caches
		run_computations();

		for (int k = 0; k < iters; k++)
		{
			perf_region_start(PERF_REGIONS_FOO, PERF_FLAG_TIMINGS | PERF_FLAG_COUNTERS);

			run_computations();

			perf_region_stop(PERF_REGIONS_FOO);

//			double fac = (double)iters * (double)size * sizeof(double);
			double fac = 1.0;

			perf_region_set_normalize(PERF_REGIONS_FOO, fac);
		}

		printf("**************************************\n");
		perf_regions_finalize();
		printf("**************************************\n");
		free(a);
	}
	return 0;
}
