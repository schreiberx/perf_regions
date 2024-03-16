#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>


#include "perf_regions.h"
#include "papi_counters.h"

#include "perf_regions_defines.h"
//#include "perf_regions_names.h"



/*
 * precompiler directives to avoid compiling with timing or perf counters
 *
 * This gets handy if perf counter PAPI is not available on certain systems.
 */
#ifndef PERF_REGIONS_USE_PAPI
#	define PERF_REGIONS_USE_PAPI		1
#endif


/***********************************************
 * Activate support for nested performance counters.
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
 *       -> accumulate performance counters		/// overheads included here for nested functions
 */
#ifndef PERF_COUNTERS_NESTED
#define PERF_COUNTERS_NESTED		1
#endif


/*
 * Use the higher-resolution POSIX clock rather than just gettimeofday()
 */
#ifndef PERF_TIMING_POSIX
#define PERF_TIMING_POSIX               1
#endif


#ifndef PERF_DEBUG
#	define PERF_DEBUG		1
#endif


#ifdef PERF_TIMING_POSIX
#include "posix_clock.h"
#endif



/**
 * Structure for each of the performance regions
 */
struct PerfRegion
{
#if PERF_REGIONS_USE_PAPI
	// storage for performance counter
	long long counter_values[PERF_COUNTERS_MAX];
#endif

	// accumulator to get total wallclock time spent in this region
	double counter_wallclock_time;

#ifdef PERF_TIMING_POSIX
	// Start time value (seconds)
	double region_enter_time_posix;
#else
	// time value
	struct timeval start_time_value;
#endif

	// region enter counter to normalize the performance counters
	double region_enter_counter;

#if PERF_COUNTERS_NESTED
	long long count_values_read[PERF_COUNTERS_MAX];

	// Set to true if performance counters could be spoiled due to nested
	int spoiled;
#endif

#if PERF_DEBUG
	// debug flag to assure that perf counters are not entering a region twice
	int active;
#endif

	// Region name
	const char* region_name;
};


struct PerfRegions
{
	// array with all regions
	struct PerfRegion *perf_regions_list;

	// Measure wallclock time
	int use_wallclock_time;

	// Measure with PAPI
	int use_papi;

	// number of performance counters
	int num_perf_counters;

	// names of performance counters
	char **perf_counter_names;


#if PERF_COUNTERS_NESTED
	// number of nested performance regions
	int num_nested_performance_regions;

	// number of maximal nested performance regions
	#define PERF_NESTED_REGIONS_MAX 16

	struct PerfRegion *nested_performance_regions[PERF_NESTED_REGIONS_MAX];
#endif


} perf_regions;


/**
 * reset the values in the performance regions
 */
void perf_regions_reset()
{
	for (int i = 0; i < PERF_REGIONS_MAX; i++)
	{
#if PERF_REGIONS_USE_PAPI

		for (int j = 0; j < perf_regions.num_perf_counters; j++)
			perf_regions.perf_regions_list[i].counter_values[j] = 0;
#endif

		perf_regions.perf_regions_list[i].counter_wallclock_time = 0;

#if PERF_COUNTERS_NESTED
		perf_regions.perf_regions_list[i].spoiled = 0;
#endif

#if PERF_DEBUG
		perf_regions.perf_regions_list[i].active = 0;
#endif
	}
}



/**
 * CONSTRUCTUR:
 *
 * Initialize the performance regions, e.g. allocate the data structures
 */
void perf_regions_init()
{
	perf_regions.perf_regions_list = 0;

	perf_regions.use_wallclock_time = 1;
	perf_regions.use_papi = 0;

#if PERF_COUNTERS_NESTED
	perf_regions.num_nested_performance_regions = 0;
#endif


	/*
	 * CONSTRUCTOR
	 */
	const char *perf_counter_list;
	{
		if (getenv("PERF_COUNTERS_LIST") == NULL)
		{
			fprintf(stderr, "PERF_COUNTERS_LIST is not defined, using no performance counters\n");
			perf_counter_list = "";

			// activating only wallclock time measurements per default
			perf_regions.use_wallclock_time = 1;
		}
		else
		{
			perf_counter_list = getenv("PERF_COUNTERS_LIST");

			// disabling wallclock time measurements per default.
			// Can be activated by adding "WALLCLOCKTIME" to PERF_COUNTERS_LIST
			perf_regions.use_wallclock_time = 0;
		}
	}

	/*
	 * Tokenize list
	 */
	char *perf_counter_list_tokens = strdup(perf_counter_list);

	// Get comma-separated events
	char *event = strtok(perf_counter_list_tokens, ",");
	while (event != NULL)
	{
		if (strcmp("WALLCLOCK", event) == 0)
		{
			perf_regions.use_wallclock_time = 1;
			continue;
		}

		perf_regions.perf_counter_names[perf_regions.num_perf_counters] = strdup(event);
		perf_regions.num_perf_counters++;

		perf_regions.use_papi = 1;

		event = strtok(NULL, ",");
	}

	papi_counters_init(perf_regions.perf_counter_names, perf_regions.num_perf_counters);

	free(perf_counter_list_tokens);

	if (perf_regions.perf_regions_list != NULL)
	{
		fprintf(stderr, "perf_regions_init called twice?");
		exit(-1);
	}

	perf_regions.perf_regions_list = malloc(sizeof(struct PerfRegion)*(PERF_REGIONS_MAX+3));


	/*
	 * RESET
	 */
	perf_regions_reset();

#if PERF_COUNTERS_NESTED
	perf_regions.num_nested_performance_regions = 0;
#endif
}



/**
 * start measuring the performance for region given by the ID.
 */
void perf_region_start(
	int i_region_id,	///< id of region
	const char* i_region_name	///< region name (associated with ID)
)
{
	struct PerfRegion *r = &(perf_regions.perf_regions_list[i_region_id]);

	// denominator to normalize values
	r->region_enter_counter++;
	r->region_name = i_region_name;

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

#if PERF_REGIONS_USE_PAPI
	if (perf_regions.use_papi)
	{
#if PERF_COUNTERS_NESTED

		perf_regions.nested_performance_regions[perf_regions.num_nested_performance_regions] = r;

		if (perf_regions.num_nested_performance_regions == 0)
		{
			papi_counters_start();
		}
		else
		{
			papi_counters_read_and_reset(r->count_values_read);

			// add performance values to outer nested performance regions
			for (int i = 0; i < perf_regions.num_nested_performance_regions; i++)
			{
				// outer performance region
				struct PerfRegion *r_outer = perf_regions.nested_performance_regions[i];

				for (int j = 0; j < perf_regions.num_perf_counters; j++)
					r_outer->counter_values[j] += r->count_values_read[j];
			}
		}

		perf_regions.num_nested_performance_regions++;

#else
		papi_counters_start();
#endif
	}


#endif

if (perf_regions.use_wallclock_time)
#ifdef PERF_TIMING_POSIX
    r->region_enter_time_posix = posix_clock();
#else
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
	struct PerfRegion *r = &perf_regions.perf_regions_list[i_region_id];

#if PERF_DEBUG
	if (r->active != 1)
	{
		printf("Region not active, but stop function called\n");
		exit(1);
	}

	r->active = 0;
#endif


#if PERF_REGIONS_USE_PAPI
	if (perf_regions.use_papi)
	{
#if PERF_COUNTERS_NESTED

		perf_regions.num_nested_performance_regions--;

		if (perf_regions.num_nested_performance_regions == 0)
		{
			long long counter_values_stop[PERF_COUNTERS_MAX];
			papi_counters_stop(counter_values_stop);

			for (int j = 0; j < perf_regions.num_perf_counters; j++)
				r->counter_values[j] += counter_values_stop[j];
		}
		else
		{
			papi_counters_read_and_reset(r->count_values_read);

			// add performance values to outer nested AND CURRENT performance region
			for (int i = 0; i <= perf_regions.num_nested_performance_regions; i++)
			{
				// outer performance region
				struct PerfRegion *r_outer = perf_regions.nested_performance_regions[i];

				for (int j = 0; j < perf_regions.num_perf_counters; j++)
					r_outer->counter_values[j] += r->count_values_read[j];

				if (i < perf_regions.num_nested_performance_regions)
					r_outer->spoiled = 1;
			}
		}

#else

		long long counter_values_stop[PERF_COUNTERS_MAX];
		papi_counters_stop(counter_values_stop);

		for (int j = 0; j < perf_regions.num_perf_counters; j++)
			r->counter_values[j] += counter_values_stop[j];

#endif
	}
#endif

	if (perf_regions.use_wallclock_time)
	{
	#ifdef PERF_TIMING_POSIX
		r->counter_wallclock_time += posix_clock() - r->region_enter_time_posix;
	#else
		struct timeval tm2;
		gettimeofday(&tm2, NULL);

		r->wallclock_time +=
					((double)time_val.tv_sec - (double)r->start_time_value.tv_sec)
					+ ((double)time_val.tv_usec - (double)r->start_time_value.tv_usec)*0.000001;
	#endif
	}
}



void perf_regions_output_human_readable_text()
{

#if PERF_REGIONS_USE_PAPI
	FILE *s = stdout;

	fprintf(s, "Performance counters profiling:\n");
	fprintf(s, "----------------------\n");
	fprintf(s, "Section");
	for (int j = 0; j < perf_regions.num_perf_counters; j++)
		fprintf(s, "\t%s", perf_regions.perf_counter_names[j]);
#if PERF_COUNTERS_NESTED
	fprintf(s, "\tSPOILED");
#endif

	if (perf_regions.use_wallclock_time)
		fprintf(s, "\tWALLCLOCKTIME");

	fprintf(s, "\tCOUNTER");
	fprintf(s, "\n");

#if PERF_DEBUG
	for (int i = 0; i < PERF_REGIONS_MAX; i++)
	{
		struct PerfRegion *r = &(perf_regions.perf_regions_list[i]);
		if (r->active != 0)
		{
			fprintf(stderr, "Still in region of %s\n", get_perf_region_name(i));
			exit(-1);
		}
	}
#endif

	for (int i = 0; i < PERF_REGIONS_MAX; i++)
	{
		struct PerfRegion *r = &(perf_regions.perf_regions_list[i]);

		if (r->region_name == 0)
			break;

		fprintf(s, "%s", r->region_name);

		for (int j = 0; j < perf_regions.num_perf_counters; j++)
		{
			long long counter_value = r->counter_values[j];

			// DON'T NORMALIZE the performance value
			//counter_value /= r->region_enter_counter;

			double param_value = counter_value;

			fprintf(s, "\t%.7e", param_value);
		}
#if PERF_COUNTERS_NESTED
		fprintf(s, "\t%i", r->spoiled);
#endif

		if (perf_regions.use_wallclock_time)
			fprintf(s, "\t%.7e", r->counter_wallclock_time);

		fprintf(s, "\t%.0f", r->region_enter_counter);

		fprintf(s, "\n");
	}
#endif

}


void perf_regions_output_csv_file()
{

}


/**
 * DECONSTRUCTOR
 */
void perf_regions_finalize()
{
	/*
	 * output performance information on each region to console
	 */
	perf_regions_output_human_readable_text();


	/*
	 * Output .csv file
	 */
	perf_regions_output_csv_file();


	/*
	 * DECONSTRUCTUR
	 */
	if (perf_regions.perf_regions_list != NULL)
	{
		free(perf_regions.perf_regions_list);
		perf_regions.perf_regions_list = NULL;
	}

	papi_counters_finalize();

#if PERF_COUNTERS_NESTED
	if (perf_regions.num_nested_performance_regions != 0)
	{
		fprintf(stderr, "num_nested_performance_regions != 0");
		exit(-1);
	}
#endif

	for (int i = 0; i < perf_regions.num_perf_counters; i++)
		free(perf_regions.perf_counter_names[i]);
}
