#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "perf_regions.h"
#include "papi_counters.h"

#include "perf_region_defines.h"
#include "perf_region_names.h"



/*
 * precompiler directives to avoid compiling with timing or perf counters
 *
 * This gets handy if perf counter PAPI is not available on certain systems.
 */
#define PERF_COUNTERS_ACTIVE		1
#define PERF_TIMINGS_ACTIVE		1

#define PERF_DEBUG		1




struct PerfRegion
{
#if PERF_COUNTERS_ACTIVE
	// storage for performance counter
	long long counter_values[PERF_COUNTERS_MAX];
#endif

#if PERF_TIMINGS_ACTIVE
	// standard timings in seconds
	double wallclock_time;

	// time value
	struct timeval tvalue;
#endif

	/*
	 *
	 * performance counting mode
	 * 0: currently not in this region
	 * PERF_TIMINGS: timings active for this region
	 * PERF_COUNTERS: counters active for this region
	 * or both
	 */
	int mode;

	// region enter counter
	int enter_counter;
};



// array with all regions
struct PerfRegion *regions = NULL;

// number of performance counters
int num_perf_counters;

// names of performance counters
char **perf_counter_names = NULL;

// values of the performance counters
long long *perf_counter_values = NULL;



/**
 * reset the values in the performance regions
 */
void perf_regions_reset()
{
	for (int i = 0; i < PERF_REGIONS_MAX; i++)
	{
		for (int j = 0; j < num_perf_counters; j++)
			regions[i].counter_values[j] = 0;

		regions[i].wallclock_time = 0;
		regions[i].mode = -1;
		regions[i].enter_counter = -1;
	}
}



/**
 * CONSTRUCTUR:
 *
 * initialize the performance regions, e.g. allocate the data structures
 */
void perf_regions_init()
{
	/*
	 * CONSTRUCTOR
	 */
	count_init();
	num_perf_counters = count_get_num();
	perf_counter_names = count_get_event_names();
	perf_counter_values = count_get_valueptr();

	if (regions != NULL)
	{
		fprintf(stderr, "perf_regions_init called twice?");
		exit(-1);
	}

	regions = malloc(sizeof(struct PerfRegion)*PERF_REGIONS_MAX);

	/*
	 * RESET
	 */
	perf_regions_reset();

	/**
	 * TODO:
	 * A) measure overheads of timing functionality
	 * B) use this in perf_regions_stop() to fix performance measurements
	 */
}





/**
 * start measuring the performance for region given by the ID.
 */
void perf_region_start(
	int i_region_id,	///< id of region
	int i_measure_type	///< type of measurements
)
{
	struct PerfRegion *r = &(regions[i_region_id]);

#if PERF_DEBUG
	if (i_region_id < 0 || i_region_id >= PERF_REGIONS_MAX)
	{
		fprintf(stderr, "Region ID %i out of boundaries\n", i_region_id);
		exit(1);
	}

	if (r->mode > 0)
	{
		fprintf(stderr, "Region activated twice\n");
		exit(1);
	}
#endif

	r->mode = i_measure_type;

#if PERF_COUNTERS_ACTIVE
	if (r->mode & PERF_FLAG_COUNTERS)
		count_start();
#endif

#if PERF_TIMINGS_ACTIVE
    gettimeofday(&(r->tvalue), NULL);
#endif
}



/**
 * stop measuring the performance for region given by the ID
 */
void perf_region_stop(
	int i_region_id		///< unique id of region
)
{
	struct PerfRegion *r = &regions[i_region_id];

#if PERF_DEBUG
	if (r->mode <= 0)
	{
		printf("Region not active, but stop function called\n");
		exit(1);
	}
#endif

	if (r->mode & PERF_FLAG_COUNTERS)
	{
		count_stop();
		for (int j = 0; j < num_perf_counters; j++)
			r->counter_values[j] += perf_counter_values[j];
	}

#if PERF_TIMINGS_ACTIVE
    struct timeval tm2;
    gettimeofday(&tm2, NULL);

    r->wallclock_time += (double)((int)tm2.tv_sec - (int)r->tvalue.tv_sec) + (double)((int)tm2.tv_usec - (int)r->tvalue.tv_usec)*0.000001;
#endif

	r->mode = 0;
}



/**
 * Output information on all performance counters
 *
 * TODO: reconstruct output from original NEMO code which looked like this:

 Total timing (sum) :
 --------------------
Elapsed Time (s)  CPU Time (s)
        79829.831    77243.499
 
Averaged timing on all processors :
-----------------------------------
Section             Elap. Time(s)  Elap. Time(%)  CPU Time(s)  CPU Time(%)  CPU/Elap Max elap(%)  Min elap(%)  Freq
sbc_ice_cice        0.2203542E+02   13.25            21.93      13.63        1.00         13.55     12.85      64.00
dia_wri             0.1621239E+02    9.75            16.15      10.04        1.00         10.88      0.56      64.00
[...]

trc_adv            -0.8005814E+01   -4.81            -8.00      -4.97        1.00         -4.60     -5.14      64.00
 
 MPI summary report :
 --------------------

 Process Rank | Elapsed Time (s) | CPU Time (s) | Ratio CPU/Elapsed
 -------------|------------------|--------------|------------------
    0         |     166.424      |     153.350  |      0.921
    1         |     166.423      |     160.846  |      0.966
[...]

  479         |     166.232      |     160.778  |      0.967
 -------------|------------------|--------------|------------------
 Total        |   79829.831      |   77243.499  |    464.449
 -------------|------------------|--------------|------------------
 Minimum      |     166.162      |     153.350  |      0.921
 -------------|------------------|--------------|------------------
 Maximum      |     166.431      |     161.694  |      0.973
 -------------|------------------|--------------|------------------
 Average      |     166.312      |     160.924  |      0.968
 
 Detailed timing for proc : 0
 --------------------------
 Section             Elapsed Time (s)  Elapsed Time (%)  CPU Time(s)  CPU Time (%)  CPU/Elapsed  Frequency
 sbc_ice_cice                  21.570            12.961       19.881        12.965        0.922         64
 trc_stp                       18.151            10.907       17.981        11.726        0.991         64
 cice_sbc_init                 11.821             7.103        5.408         3.527        0.458          1
 [...]
 trc_adv                       -7.804            -4.689       -7.788        -5.079        0.998         64

 Timing started on 19/05/2016 at 13:55:48 MET +00:00 from GMT
 Timing   ended on 19/05/2016 at 13:58:34 MET +00:00 from GMT
 */
void perf_regions_output(FILE *s)
{
#if PERF_TIMINGS_ACTIVE
	fprintf(s, "wallclocktime");
#endif

#if PERF_COUNTERS_ACTIVE
	for (int j = 0; j < num_perf_counters; j++)
		fprintf(s, "\t%s", perf_counter_names[j]);
	fprintf(s, "\n");
#endif


#if PERF_DEBUG
	for (int i = 0; i < PERF_REGIONS_MAX; i++)
	{
		struct PerfRegion *r = &(regions[i]);
		if (r->mode > 0)
		{
			fprintf(stderr, "Still in region of %s\n", get_perf_region_name(i));
			exit(-1);
		}
	}
#endif


	/**
	 * TODO: Do fancy output here
	 */
	for (int i = 0; i < PERF_REGIONS_MAX; i++)
	{
		struct PerfRegion *r = &(regions[i]);

		if (regions[i].mode == -1)
			continue;

		const char *perf_region_name = get_perf_region_name(i);
		if (perf_region_name == 0)
		{
			fprintf(stderr, "String for performance region %i in perf_region_names.c not set!\n", i);
			exit(1);
		}

		fprintf(s, "%s", perf_region_name);
		fprintf(s, "\t%f", r->wallclock_time);
		for (int j = 0; j < num_perf_counters; j++)
			fprintf(s, "\t%lld", r->counter_values[j]);
		fprintf(s, "\n");
	}
}



/**
 * DECONSTRUCTOR
 */
void perf_regions_finalize()
{
	/*
	 * where to write the performance data to
	 */
	FILE *s = stdout;


	/*
	 * output performance information on each region
	 */
	perf_regions_output(s);


	/*
	 * DECONSTRUCTUR
	 */
	if (regions != NULL)
	{
		free(regions);
		regions = NULL;
	}

	count_finalize();
}
