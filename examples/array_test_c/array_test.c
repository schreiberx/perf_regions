#include <stdio.h>
#include <stdlib.h>

//PERF_REGION_ORIGINAL
//#pragma perf_region include
//PERF_REGION_CODE
#include <perf_regions.h>

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

//PERF_REGION_ORIGINAL
//#pragma perf_region init
//PERF_REGION_CODE
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

//PERF_REGION_ORIGINAL
//#pragma perf_region start foo
//PERF_REGION_CODE
perf_region_start(0, (PERF_FLAG_TIMINGS | PERF_FLAG_COUNTERS)); //FOO


			run_computations();
//PERF_REGION_ORIGINAL
//#pragma perf_region stop foo
//PERF_REGION_CODE
perf_region_stop(0); //FOO

//PERF_REGION_ORIGINAL
//#pragma perf_region start bar
//PERF_REGION_CODE
perf_region_start(1, (PERF_FLAG_TIMINGS | PERF_FLAG_COUNTERS)); //BAR
			run_computations();
			run_computations();
//PERF_REGION_ORIGINAL
//#pragma perf_region stop bar
//PERF_REGION_CODE
perf_region_stop(1); //BAR

//			double fac = (double)iters * (double)size * sizeof(double);
			double fac = 1.0;

//			perf_region_set_normalize(PERF_REGIONS_FOO, fac);
		}

//PERF_REGION_ORIGINAL
//#pragma perf_region finalize
//PERF_REGION_CODE
perf_regions_finalize();

	free(a);
	}
	return 0;
}