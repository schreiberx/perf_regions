#include <stdio.h>
#include <stdlib.h>

#pragma perf_region include

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

#pragma perf_region init

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

#pragma perf_region start foo


			run_computations();
#pragma perf_region stop foo

#pragma perf_region start bar
			run_computations();
			run_computations();
#pragma perf_region stop bar

//			double fac = (double)iters * (double)size * sizeof(double);
			double fac = 1.0;

//			perf_region_set_normalize(PERF_REGIONS_FOO, fac);
		}

#pragma perf_region finalize

	free(a);
	}
	return 0;
}