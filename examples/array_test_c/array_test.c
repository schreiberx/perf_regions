#include <stdio.h>
#include <stdlib.h>

#pragma perf_regions include

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

#pragma perf_regions init
#pragma perf_regions start outer

		printf("\n");
		printf("**************************************\n");
		printf("Mem size: %f KByte\n", (double)size*0.001);
		printf("Iterations: %i\n", iters);

		// initialize everything
		for (int i = 0; i < size; i++)
			a[i] = i;

		// dummy computations to warmup caches
		run_computations();
		for (int k = 0; k < iters; k++)
		{

#pragma perf_regions start foo


			run_computations();
#pragma perf_regions stop foo

#pragma perf_regions start bar
			run_computations();
			run_computations();
#pragma perf_regions stop bar

//			perf_region_set_normalize(PERF_REGIONS_FOO, fac);
		}

#pragma perf_regions stop outer
#pragma perf_regions finalize

	free(a);
	}
	return 0;
}