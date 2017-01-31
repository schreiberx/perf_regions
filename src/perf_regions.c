#include <stdlib.h>
#include <stdio.h>
#include <time.h>
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

/*
 * Activate support for recursive performance counters.
 * Warning: This leads to additional overheads and
 * might result in decreased accurate results!
 *
 ***********************************************
 * Recursion not activated:
 *
 *  perf_region_start();
 *    -> count_start();
 *       (reset and start counters)
 *
 *  perf_region_stop();
 *    -> count_stop();
 *       (stop counters)
 *    -> accumulate performance counters
 *
 *
 ***********************************************
 * Recursion activated:
 *
 *  perf_region_init();
 *       (reset and start counters)
 *
 *  perf_region_start();
 *    if num_nested_performance_regions == 0:
 *        -> count_start();
 *
 *    else:
 *        -> count_read_and_reset();
 *           (read counters)
 *           (update performance counters of outer regions)
 *    num_nested_performance_regions++;
 *
 *
 *  perf_region_stop();
 *    num_nested_performance_regions--;
 *
 *    if num_nested_performance_regions == 0:
 *       -> count_stop();
 *          (stop counters)
 *
 *       -> accumulate performance counters
 *
 *    else:
 *       -> count_read_and_reset();
 *          (read counters)
 *          (update performance counters of outer regions)
 *
 *       -> accumulate performance counters		/// overheads included here for recursive functions
 */
#define PERF_COUNTERS_RECURSIVE		1

/*
 *
 */
#define PERF_TIMINGS_ACTIVE		1

#ifndef PERF_DEBUG
#	define PERF_DEBUG		1
#endif





/**
 * Structure for each of the performance regions
 */
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
	struct timeval start_time_value;

#endif

	/*
	 * performance counting mode
	 * PERF_TIMINGS: timings active for this region
	 * PERF_COUNTERS: counters active for this region
	 * or both
	 */
	int mode;

	// region enter counter to normalize the performance counters
	double region_enter_counter_normalize_denom;

#if PERF_COUNTERS_RECURSIVE
	long long count_values_read[PERF_COUNTERS_MAX];
//	long long count_values_read_end[PERF_COUNTERS_MAX];
#endif

#if PERF_DEBUG
	// debug flag to assure that perf counters are not entering a region twice
	int active;
#endif
};

#if PERF_TIMINGS_ACTIVE
	// start and end dates
	time_t wallclock_pretty_init_time, wallclock_pretty_end_time;

	// start and end time
	struct timeval wallclock_init_time, tm_end;
	// total time in seconds
	double wallclock_tot_time;
#endif


// array with all regions
struct PerfRegion *regions = NULL;


// number of performance counters
int num_perf_counters;

// names of performance counters
char **perf_counter_names = NULL;


// number of nested performance regions
static int num_nested_performance_regions;

#if PERF_COUNTERS_RECURSIVE
	// number of maximal recursive performance regions
	#define PERF_RECURSIVE_REGIONS_MAX 16

	struct PerfRegion *nested_performance_regions[PERF_RECURSIVE_REGIONS_MAX];
#endif

long long counter_values_stop[PERF_COUNTERS_MAX];


/**
 * reset the values in the performance regions
 */
void perf_regions_reset()
{
	for (int i = 0; i < PERF_REGIONS_MAX; i++)
	{
#if PERF_COUNTERS_ACTIVE

		for (int j = 0; j < num_perf_counters; j++)
			regions[i].counter_values[j] = 0;
#endif

#if PERF_TIMINGS_ACTIVE
		regions[i].wallclock_time = 0;
#endif

		regions[i].mode = -1;
		regions[i].region_enter_counter_normalize_denom = 0;

#if PERF_DEBUG
		regions[i].active = 0;
#endif
	}
}



/**
 * CONSTRUCTUR:
 *
 * initialize the performance regions, e.g. allocate the data structures
 */
void perf_regions_init()
{
	int i, j, region_id;
	int istart_count;

	/*
	 * CONSTRUCTOR
	 */

	count_init();
	num_perf_counters = count_get_num();
	perf_counter_names = count_get_event_names();

	if (regions != NULL)
	{
		fprintf(stderr, "perf_regions_init called twice?");
		exit(-1);
	}

	regions = malloc(sizeof(struct PerfRegion)*(PERF_REGIONS_MAX+3));

	/*
	 * Load list with perf region names
	 */
	perf_region_name_init();


	/*
	 * RESET
	 */
	perf_regions_reset();


	/**
	 * measure overheads
	 *
	 * We have 3 different combinations to measure the performance and we measure all of them.
	 */

	// 100 overhead measurements
	for (i = 0; i < 100; i++)
	{
		perf_region_start(PERF_REGIONS_OVERHEAD_TIMINGS, PERF_FLAG_TIMINGS);
		perf_region_stop(PERF_REGIONS_OVERHEAD_TIMINGS);
	}

	for (i = 0; i < 100; i++)
	{
		perf_region_start(PERF_REGIONS_OVERHEAD_COUNTERS, PERF_FLAG_COUNTERS);
		perf_region_stop(PERF_REGIONS_OVERHEAD_COUNTERS);
	}

	for (i = 0; i < 100; i++)
	{
		perf_region_start(PERF_REGIONS_OVERHEAD_TIMINGS_COUNTERS, PERF_FLAG_TIMINGS | PERF_FLAG_COUNTERS);
		perf_region_stop(PERF_REGIONS_OVERHEAD_TIMINGS_COUNTERS);
	}

	for (	region_id = PERF_REGIONS_OVERHEAD_TIMINGS_COUNTERS;
			region_id <= PERF_REGIONS_OVERHEAD_TIMINGS;
			region_id++
	)
	{
		struct PerfRegion *r = &regions[region_id];

		/*
		 * Normalize everything
		 */
#if PERF_TIMINGS_ACTIVE
		r->wallclock_time /= (double)(r->region_enter_counter_normalize_denom);
#endif

#if PERF_COUNTERS_ACTIVE
		for (j = 0; j < num_perf_counters; j++)
			r->counter_values[j] /= (double)(r->region_enter_counter_normalize_denom);
#endif

		r->region_enter_counter_normalize_denom = 1.0;
	}

#if PERF_TIMINGS_ACTIVE
	// start time measurement
	gettimeofday(&wallclock_init_time, NULL);
#endif

	// start date measurement
	time(&wallclock_pretty_init_time);

	num_nested_performance_regions = 0;
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

	// denominator to normalize values
	r->region_enter_counter_normalize_denom++;

#if PERF_DEBUG

	if (i_region_id < 0 || i_region_id >= PERF_REGIONS_MAX)
	{
		fprintf(stderr, "Region ID %i out of boundaries\n", i_region_id);
		exit(1);
	}

	if (r->active > 0)
	{
		fprintf(stderr, "Region activated twice\n");
		exit(1);
	}

	r->active = 1;

#endif

	r->mode = i_measure_type;

#if PERF_COUNTERS_ACTIVE

	if (r->mode & PERF_FLAG_COUNTERS)
	{
#if PERF_COUNTERS_RECURSIVE

		nested_performance_regions[num_nested_performance_regions] = r;

		if (num_nested_performance_regions == 0)
		{
			count_start();
		}
		else
		{
			count_read_and_reset(r->count_values_read);

			// add performance values to outer recursive performance regions
			for (int i = 0; i < num_nested_performance_regions; i++)
			{
				// outer performance region
				struct PerfRegion *r_outer = nested_performance_regions[i];

				for (int j = 0; j < num_perf_counters; j++)
					r_outer->counter_values[j] += r->count_values_read[j];
			}
		}

		num_nested_performance_regions++;

#else
		count_start();
#endif
	}


#endif

#if PERF_TIMINGS_ACTIVE
    gettimeofday(&(r->start_time_value), NULL);
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
	if (r->mode <= 0 || r->active != 1)
	{
		printf("Region not active, but stop function called\n");
		exit(1);
	}

	r->active = 0;
#endif


#if PERF_COUNTERS_ACTIVE
	if (r->mode & PERF_FLAG_COUNTERS)
	{

#if PERF_COUNTERS_RECURSIVE

		num_nested_performance_regions--;

		if (num_nested_performance_regions == 0)
		{
			count_stop(counter_values_stop);

			for (int j = 0; j < num_perf_counters; j++)
				r->counter_values[j] += counter_values_stop[j];
		}
		else
		{
			count_read_and_reset(r->count_values_read);

			// add performance values to outer recursive AND CURRENT performance region
			for (int i = 0; i <= num_nested_performance_regions; i++)
			{
				// outer performance region
				struct PerfRegion *r_outer = nested_performance_regions[i];

				for (int j = 0; j < num_perf_counters; j++)
					r_outer->counter_values[j] += r->count_values_read[j];
			}
		}

#else

		count_stop(counter_values_stop);

		for (int j = 0; j < num_perf_counters; j++)
			r->counter_values[j] += counter_values_stop[j];

#endif

	}
#endif

#if PERF_TIMINGS_ACTIVE
    struct timeval time_val;
    gettimeofday(&time_val, NULL);

    double test = ((double)time_val.tv_sec - (double)r->start_time_value.tv_sec) + ((double)time_val.tv_usec - (double)r->start_time_value.tv_usec)*0.000001;
    printf("%i STOP\t%e\t%e\n", i_region_id, r->wallclock_time, test);

    r->wallclock_time +=
    			  ((double)time_val.tv_sec - (double)r->start_time_value.tv_sec)
				+ ((double)time_val.tv_usec - (double)r->start_time_value.tv_usec)*0.000001;
#endif

    // don't overwrite mode since we need this information for the overheads later
//	r->mode = 0;
}



/**
 * Output information on all performance counters
 *
 * TODO: reconstruct output from original NEMO code which looked like this:
 * Performance counter and timing sections have been added. Missing info at this stage:
 * 1. the section related to MPI (we are running a sequential version of the code)
 * 2. the timing info are provided by the gettimeofday function, so we don't have elapsed and cpu time as in NEMO

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
	char* time_string;


#if PERF_COUNTERS_ACTIVE

/*
 * Performance counters output

Performance counters profiling:
----------------------
Section			PAPI_L1_TCM	PAPI_L2_TCM	PAPI_L3_TCM
FOO			2.6235250e+05	1.5653000e+04	2.5350000e+02
*/

#if PERF_COUNTERS_ACTIVE
	fprintf(s, "Performance counters profiling:\n");
	fprintf(s, "----------------------\n");
	fprintf(s, "Section\t\t");
	for (int j = 0; j < num_perf_counters; j++)
		fprintf(s, "\t%s", perf_counter_names[j]);
	fprintf(s, "\n");
#endif

#if PERF_DEBUG
	for (int i = 0; i < PERF_REGIONS_MAX; i++)
	{
		struct PerfRegion *r = &(regions[i]);
		if (r->active != 0)
		{
			fprintf(stderr, "Still in region of %s\n", get_perf_region_name(i));
			exit(-1);
		}
	}
#endif

	for (int i = 0; i < PERF_REGIONS_MAX-3; i++)
	{
		struct PerfRegion *r = &(regions[i]);

		if (regions[i].mode == -1)
			continue;

		/*
		 * determine which overhead measurements to use for this performance region
		 */
		struct PerfRegion *r_overhead = NULL;
		switch (regions[i].mode)
		{
		case PERF_FLAG_TIMINGS:
			r_overhead = &regions[PERF_REGIONS_OVERHEAD_TIMINGS];
			break;

		case PERF_FLAG_COUNTERS:
			r_overhead = &regions[PERF_REGIONS_OVERHEAD_COUNTERS];
			break;

		case (PERF_FLAG_TIMINGS | PERF_FLAG_COUNTERS):
			r_overhead = &regions[PERF_REGIONS_OVERHEAD_TIMINGS_COUNTERS];
			break;

		default:
			fprintf(s, "INVALID MODE id %i!\n", regions[i].mode);
			exit(1);
		}

		const char *perf_region_name = get_perf_region_name(i);
		if (perf_region_name == 0)
		{
			fprintf(stderr, "String for performance region %i in perf_region_names.c not set!\n", i);
			exit(1);
		}

#if PERF_TIMINGS_ACTIVE
		// decrease by n-times overheads
//		r->wallclock_time -= r_overhead->wallclock_time*r->region_enter_counter_normalize_denom;

//		wallclock_tot_time -= r_overhead->wallclock_time*r->region_enter_counter_normalize_denom;
#endif

		fprintf(s, "%s\t\t", perf_region_name);

		for (int j = 0; j < num_perf_counters; j++)
		{
			long long counter_value = r->counter_values[j];
			counter_value -= r_overhead->counter_values[j]*r->region_enter_counter_normalize_denom;

			// DON'T NORMALIZE the performance value
			//counter_value /= r->region_enter_counter_normalize_denom;

			fprintf(s, "\t%.7e", counter_value);
		}
		fprintf(s, "\n");
	}
#endif

/* Timing output
 
 Performance counters profiling:

Total timing (sum) :
----------------------
Wallclock time (s)
1.9277000e-02

Timing profiling:
----------------------
Section		Wallclock time(sec)		Wallclock time(%)		Frequency
FOO		1.7152000e-02			88.98				2

*/

#if PERF_TIMINGS_ACTIVE
	int last_c;
	int end_ord;
	struct PerfRegion tmp;


#if 0
	// MaS: Commented, is this a sorting operation?
	//
	// TO BE TESTED ON MORE THAN ONE REGION
	end_ord = PERF_REGIONS_MAX-3;
	while (end_ord != 0)
	{
		last_c = 0;
		for (int i = 0; i < end_ord; i++)
		{
			int j = i + 1;
			if (regions[i].mode == -1)
				continue;

			if (regions[j].mode == -1)
				j++;

			if (regions[i].wallclock_time < regions[j].wallclock_time)
			{
				tmp = regions[j];
				regions[j] = regions[i];
				regions[i] = tmp;
				last_c = i;
			}
		}
		end_ord = last_c;
	}
#endif


	fputs("\n\n", s);
	fputs(" Total timing (sum) :\n", s);
	fputs("----------------------\n", s);
	fputs("Wallclock time (s)\n", s);
	fprintf(s, "%.7e\n\n", wallclock_tot_time);


	fputs("Timing profiling:\n", s);
	fputs("----------------------\n", s);

	// TODO: Rename section to Region?
	fputs("Section\t\tWallclock time(sec)\t\tWallclock time(%)\t\tFrequency\n", s);

	for (int i = 0; i < PERF_REGIONS_MAX-3; i++)
    {
		if (regions[i].mode == -1)
			continue;

		fprintf(s, "%s\t\t", get_perf_region_name(i));
		fprintf(s, "%.7e\t\t\t", regions[i].wallclock_time);
		fprintf(s, "%.2f\t\t\t\t", regions[i].wallclock_time/wallclock_tot_time*100);
		fprintf(s, "%.0f\n", regions[i].region_enter_counter_normalize_denom);
	}

#endif

/*
Timing started on Tue Sep 27 18:50:35 2016
Timing   ended on Tue Sep 27 18:50:35 2016
*/

	time_string = ctime(&wallclock_pretty_init_time);
	fprintf(s, "\nTiming started on %s", time_string);
	time_string = ctime(&wallclock_pretty_end_time);
	fprintf(s, "Timing   ended on %s", time_string);
}

/**
 * Set scalar to multiply the output performance values with.
 */
void perf_region_set_normalize(
		int i_region_id,			///< unique id of region
		double i_normalize_denom
)
{
	struct PerfRegion *r = &(regions[i_region_id]);

	r->region_enter_counter_normalize_denom = i_normalize_denom;
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

#if PERF_TIMINGS_ACTIVE
	// end time measurement
	gettimeofday(&tm_end, NULL);

	// total time measurement (in seconds)
	wallclock_tot_time =
    			  ((double)tm_end.tv_sec - (double)wallclock_init_time.tv_sec)
				+ ((double)tm_end.tv_usec - (double)wallclock_init_time.tv_usec)*0.000001;

	time(&wallclock_pretty_end_time);
#endif

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

	/*
	 * performance region names
	 */
	perf_region_name_shutdown();

	count_finalize();

	if (num_nested_performance_regions != 0)
	{
		fprintf(stderr, "num_nested_performance_regions != 0");
		exit(-1);
	}
}
